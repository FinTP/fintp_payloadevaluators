/*
* FinTP - Financial Transactions Processing Application
* Copyright (C) 2013 Business Information Systems (Allevo) S.R.L.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>
* or contact Allevo at : 031281 Bucuresti, 23C Calea Vitan, Romania,
* phone +40212554577, office@allevo.ro <mailto:office@allevo.ro>, www.allevo.ro.
*/

#include <sstream>
#include <deque>

#include "Trace.h"
using namespace FinTP;

#include "WSRM/SequenceAcknowledgement.h"
#include "WSRM/SequenceFault.h"

#include "SepaStsPayload.h"

const string SepaStsPayload::m_StatusXpath = "//x:GrpSts/text()";

SepaStsPayload::SepaStsPayload( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* document ) :
	m_BatchType( 2 ), m_AckType( 2), m_NackType( 2 ),RoutingMessageEvaluator( document, RoutingMessageEvaluator::SAGSTS )
{
	m_SequenceResponse = NULL;

	const DOMElement* root = m_Document->getDocumentElement();
	for ( XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* key = root->getFirstChild(); key != 0; key = key->getNextSibling() )
	{
		if ( key->getNodeType() != DOMNode::ELEMENT_NODE )
			continue;

		m_MessageType = localForm( key->getLocalName() );
	}
}

SepaStsPayload::~SepaStsPayload()
{
	try
	{
		if ( m_SequenceResponse != NULL )
			delete m_SequenceResponse;
		m_SequenceResponse = NULL;
	}
	catch( ... )
	{
		try
		{
			TRACE( "An error occured while deleting sequence response" );
		} catch( ... ) {}
	}
}

string SepaStsPayload::internalToString()
{
	return XmlUtil::SerializeToString( m_Document );
}

const RoutingAggregationCode& SepaStsPayload::getAggregationCode( const RoutingAggregationCode& feedback )
{
	//Deocamdata: Corelarea e controlata de suprascrierea feedbackului in RouteBatchReply practic pana sa se ajnga la ApplyQueueRouting cand se va executa Aggregate
	//TODO: Add something for outgoing reject messages
	m_AggregationCode.Clear();
	string correlId = feedback.getCorrelId();
	string correlToken = feedback.getCorrelToken();
	m_AggregationCode.setCorrelToken( correlToken );
	m_AggregationCode.setCorrelId( correlId );
	string code = "";
	string aggField = RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE ;
	
	/** RJCT/RJCT came in by transaction
	 *  PART/RJCT came as batch but reactivated
	 *  PART/ACSC came as batch but reactivated
	 */

	if ( correlToken == RoutingMessageEvaluator::AGGREGATIONTOKEN_TRN )
	{
		if( feedback.containsAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE ) )
			code = feedback.getAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE );
		else if( isNack() )
		{
			wsrm::SequenceFault* response = dynamic_cast< wsrm::SequenceFault* >( SepaStsPayload::getSequenceResponse() );
			if( response == NULL )
				throw logic_error( "SepaStsPayload is not a fault sequence" );
			if ( response->IsNack( correlId ) )
				code = response->getNack( correlId ).getErrorCode();
			else //PART/RJCT but not rejected
			{
				aggField = RoutingMessageEvaluator::AGGREGATIONTOKEN_SAACODE;
				code = RoutingMessageEvaluator::FEEDBACKFTP_APPROVED;
			}

		}
		else if( isAck() )
		{
			wsrm::SequenceAcknowledgement* response = dynamic_cast< wsrm::SequenceAcknowledgement* >( SepaStsPayload::getSequenceResponse() );
			if( response == NULL )
				throw logic_error( "SepaStsPaylod is not sequence acknowledgement" );
			if( response->IsAck( correlId ) )
				code = RoutingMessageEvaluator::FEEDBACKFTP_ACK;
		}
		else
			code = getOverrideFeedback();

		m_AggregationCode.addAggregationField( aggField, code );
		m_AggregationCode.addAggregationCondition( RoutingMessageEvaluator::AGGREGATIONTOKEN_BATCHID, getField( InternalXmlPayload::RELATEDREF ) );
		m_AggregationCode.addAggregationCondition( "TFDCODE IS NULL", "" );
	}

	/** ACCP came in as batch
	 *  PART/ACSC with all transactions specified
	 */
	if ( correlToken == RoutingMessageEvaluator::AGGREGATIONTOKEN_BATCHID )
	{
		if( isAck() )
		{
			m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE, RoutingMessageEvaluator::FEEDBACKFTP_ACK );
		}
		else
		{
			if( feedback.containsAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_SAACODE ) )
				code = feedback.getAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_SAACODE );
			else
				code = getOverrideFeedback();
			m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_SAACODE, code );
		}
		m_AggregationCode.addAggregationCondition( RoutingMessageEvaluator::AGGREGATIONTOKEN_ISSUER, getField( InternalXmlPayload::RECEIVER) );
	}

	//PART/RJCT but not rejected
	/*
	if ( correlToken == RoutingMessageEvaluator::AGGREGATIONTOKEN_FTPID )
	{
		m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_SAACODE, RoutingMessageEvaluator::FEEDBACKFTP_APPROVED );
	}
	*/
	//reject from qPI
	if ( correlToken == RoutingMessageEvaluator::AGGREGATIONTOKEN_MQID )
	{
		//not realy good feedback information; to overwrite feedback
		m_AggregationCode.setCorrelToken( getOverrideFeedbackId() );
		m_AggregationCode.setCorrelId( getField( InternalXmlPayload::ORGTXID ) );
		m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE, getOverrideFeedback() );

	}

	return m_AggregationCode;
}

bool SepaStsPayload::isReply()
{
	return true;
}

bool SepaStsPayload::isAck()
{

	// if it was evaluated, its value is bool
	if ( m_AckType != 2 )
		return ( m_AckType == 1 );
	else
	{
		//transactions status are needed to decide
		if( isBatch() )
			getSequenceResponse();
		else
		{
			string status = getCustomXPath( m_StatusXpath );
			if( status == "ACSC" )
				m_AckType = 1;
			else
				m_AckType = 0;
		}
	}

	return( m_AckType == 1 );
}

bool SepaStsPayload::isNack()
{
	if ( m_NackType != 2 )
		return ( m_NackType == 1 );
	else
	{
		//transactions status are needed to decide
		if( isBatch() )
			getSequenceResponse();
		else
		{
			string status = getCustomXPath( m_StatusXpath );
			if ( status == "RJCT" )
				m_NackType = 1;
			else
				m_NackType = 0;
		}
		return( m_NackType == 1 );
	}
}

bool SepaStsPayload::isBatch()
{
	// if it was evaluated, its value is bool
	if ( m_BatchType != 2 )
		return ( m_BatchType == 1 );

	string grpStatus = getCustomXPath( m_StatusXpath );
	if( ( grpStatus == "PART" ) || ( grpStatus == "ACCP" ) )
		m_BatchType = 1;
	else
		m_BatchType = 0;

	return ( m_BatchType == 1 );
}

string SepaStsPayload::getOverrideFeedback()
{
	if( isNack() )
	{
		string codePath = "//x:TxInfAndSts/x:StsRsnInf/x:Rsn/x:Cd/text()";
		return XPathHelper::SerializeToString( XPathHelper::Evaluate( codePath, m_XalanDocument, m_Namespace ) );
	}

	if( isAck() )
		return RoutingMessageEvaluator::FEEDBACKFTP_ACK;

	string status = getCustomXPath( m_StatusXpath );
	if( status == "ACCP" )
		return RoutingMessageEvaluator::FEEDBACKFTP_APPROVED;
	return status;
}

RoutingMessageEvaluator::FeedbackProvider SepaStsPayload::getOverrideFeedbackProvider()
{
	if( isBatch() )
		return RoutingMessageEvaluator::FEEDBACKPROVIDER_TFD;
	return RoutingMessageEvaluator::FEEDBACKPROVIDER_UNK;
}

string SepaStsPayload::getOverrideFeedbackId()
{
	if( ( isBatch() && isNack() ) || ( getCustomXPath( m_StatusXpath ) == "ACCP" ) )
		return getField( InternalXmlPayload::RELATEDREF );
	if( isNack() || isAck() )
		return getField( InternalXmlPayload::ORGTXID );
	return "";
}

// TODO: check if m_seqRespones is for no use outside; if so just tace the code from payload
// not used yet
wsrm::SequenceResponse* SepaStsPayload::getSequenceResponse()
{
	if ( m_SequenceResponse != NULL )
		return m_SequenceResponse;

	//already evaluated as indefinite reply no SequenceResponse
	if ( m_AckType == 0 && m_NackType == 0 )
		return NULL;

	string stsType = getCustomXPath( m_StatusXpath );
	string batchId = getField( InternalXmlPayload::RELATEDREF );
	string batchIssuer = getField( InternalXmlPayload::SENDER );
	string prevStatus = "";

	if( stsType == "ACCP" )
	{
		m_SequenceResponse = new wsrm::SequenceAcknowledgement( batchId );
		m_AckType = 0;
		m_NackType = 0;
		return m_SequenceResponse;
	}

	const DOMElement* root = m_Document->getDocumentElement();

	DOMNodeList* txnInfNodes = root->getElementsByTagName( unicodeForm( "TxInfAndSts" ) );
	if ( ( txnInfNodes == NULL ) || ( txnInfNodes->getLength() == 0 ) )
		throw runtime_error( "Missing required [TxInfAndSts] element in [FIToFIPmtStsRpt] document" );

	// individual item replied
	DEBUG( "Some items in batch [" << batchId << "] issued by [" << batchIssuer << "] were replied. Iterating through individual replies..." );

	for ( unsigned int i = 0; i < txnInfNodes->getLength(); i++ )
	{
		const DOMElement* txnInfNode = dynamic_cast< DOMElement* >( txnInfNodes->item( i ) );
		if ( txnInfNode == NULL )
			throw logic_error( "Bad type : [FIToFIPmtStsRpt/TxInfAndSts] should be an element" );

		DOMNodeList* txStsNodes = txnInfNode->getElementsByTagName( unicodeForm( "TxSts" ) );
		if ( ( txStsNodes == NULL ) || ( txStsNodes->getLength() == 0 ) )
			throw runtime_error( "Missing required [TxSts] element in [FIToFIPmtStsRpt/TxInfAndSts] " );
		const DOMElement* txStsNode = dynamic_cast< DOMElement* >( txStsNodes->item( 0 ) );
		if ( txStsNode == NULL )
			throw logic_error( "Bad type : [FIToFIPmtStsRpt/TxInfAndSts/TxSts] should be an element" );

		DOMText* txStsType = dynamic_cast< DOMText* >( txStsNode->getFirstChild() );
		string statusType = localForm( txStsType->getData() );
		if( i == 0 )
		{
			if( statusType == "RJCT")
			{
				m_SequenceResponse = new wsrm::SequenceFault( batchId );
				m_AckType = 0;
				m_NackType = 1;
			}
			else if( statusType == "ACSC")
			{
				m_SequenceResponse = new wsrm::SequenceAcknowledgement( batchId );
				m_AckType = 1;
				m_NackType = 0;
			}
			else
			{
				//indefinite reply no SequenceResponse
				m_AckType = 0;
				m_NackType = 0;
				return NULL;
				//stringstream errorMessage;
				//errorMessage << "Replied batch [" << batchId << " ] is't ack nor nack. Transaction found status: [" << statusType << "] ";
				//throw logic_error( errorMessage.str() );
			}
			prevStatus = statusType;
		}

		if( prevStatus != statusType )
		{
			stringstream errorMessage;
			errorMessage << "Replied batch [" << batchId << " ] with " << prevStatus << " transactions, contains " << statusType << " transactions";
			throw logic_error( errorMessage.str() );
		}

		DOMNodeList* txRefNodes = txnInfNode->getElementsByTagName( unicodeForm( "OrgnlTxId" ) );
		if ( ( txRefNodes == NULL ) || ( txRefNodes->getLength() == 0 ) )
			throw runtime_error( "Missing required [OrgnlTxId] element in [FIToFIPmtStsRpt/TxInfAndSts] " );
		const DOMElement* txRefNode = dynamic_cast< DOMElement* >( txRefNodes->item( 0 ) );
		if ( txRefNode == NULL )
			throw logic_error( "Bad type : [FIToFIPmtStsRpt/TxInfAndSts/OrgnlTxId] should be an element" );
		DOMText* txRef = dynamic_cast< DOMText* >( txRefNode->getFirstChild() );
		string originalRef = localForm( txRef->getData() );

		if( statusType == "RJCT" )
		{
			DOMNodeList* txCdNodes = txnInfNode->getElementsByTagName( unicodeForm( "Cd" ) );
			if ( ( txCdNodes == NULL ) || ( txCdNodes->getLength() == 0 ) )
				throw runtime_error( "Missing required [Cd] element in [FIToFIPmtStsRpt/TxInfAndSts/StsRsnInf/Rsn] " );
			const DOMElement* txCdNode = dynamic_cast< DOMElement* >( txCdNodes->item( 0 ) );
			if ( txCdNode == NULL )
				throw logic_error( "Bad type : [FIToFIPmtStsRpt/TxInfAndSts/StsRsnInf/Rsn/Cd] should be an element" );
			DOMText* txCd = dynamic_cast< DOMText* >( txCdNode->getFirstChild() );
			string txCode = localForm( txCd->getData() );

			wsrm::Nack nack( txCode );
			m_SequenceResponse->AddChild( originalRef, nack );
		}
		if( statusType == "ACSC")
		{
			wsrm::Ack ack ;
			m_SequenceResponse->AddChild( originalRef, ack );
		}
		prevStatus = statusType;
	}
	DEBUG( "Returning response..." );
	return m_SequenceResponse;
}
/*
bool SepaStsPayload::updateRelatedMessages()
{
	string originalBatchId = getField( InternalXmlPayload::RELATEDREF );
	if( isBatch() && isAck() )
		return ( originalBatchId.length() > 0 );

	return false;
}
*/

RoutingAggregationCode SepaStsPayload::getBusinessAggregationCode()
{
	RoutingAggregationCode businessReply( RoutingMessageEvaluator::AGGREGATIONTOKEN_TRN, getField( InternalXmlPayload::TRN ) );
	/*
	string originalBatchId = getField( InternalXmlPayload::RELATEDREF );
	if( originalBatchId.length() > 0 )
	{
		string messageType = getField( InternalXmlPayload::MESSAGETYPE );
		string currency = getField( InternalXmlPayload::CURRENCY );
		string amount = RoutingEngine::EvaluateKeywordValue( messageType, currency, "Currency", "ammount" );

		businessReply.addAggregationCondition( RoutingMessageEvaluator::AGGREGATIONTOKEN_BATCHID, originalBatchId );

		Currency amountParsed( amount );
		if ( amountParsed.isZero() )
		{
			businessReply.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE, RoutingMessageEvaluator::FEEDBACKFTP_REFUSE );
		}
		else
		{
			businessReply.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_SWIFTCODE,	RoutingMessageEvaluator::FEEDBACKFTP_REFUSE );
		}
	}
	*/
	return businessReply;

}

string SepaStsPayload::getIssuer()
{
	//issuer of the original messages
	return getField( InternalXmlPayload::RECEIVER);
}

string SepaStsPayload::getMessageType()
{
	return m_MessageType;
}