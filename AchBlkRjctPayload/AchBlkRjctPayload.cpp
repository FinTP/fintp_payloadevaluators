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

#include <xercesc/dom/DOM.hpp>
#include <xalanc/XPath/NodeRefList.hpp>

#include "Trace.h"

#include "WSRM/SequenceAcknowledgement.h"
#include "WSRM/SequenceFault.h"

#include "AchBlkRjctPayload.h"
#include "RoutingExceptions.h"

const string AchBlkRjctPayload::OUTGOINGBATCHTYPE = "OBatchType";
const string AchBlkRjctPayload::INCOMINGBATCHTYPE = "IBatchType";

AchBlkRjctPayload::AchBlkRjctPayload( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* document ) :
	RoutingMessageEvaluator( document, RoutingMessageEvaluator::ACHBLKRJCT ), m_NackType( 2 )
{
	m_SequenceResponse = NULL;

	const DOMElement* root = m_Document->getDocumentElement();

	DOMNodeList* addRefsNodes = root->getElementsByTagName( unicodeForm( "AddtlRefs" ) );
	if ( ( addRefsNodes == NULL ) || ( addRefsNodes->getLength() == 0 ) )
		throw runtime_error( "Missing required [AddtlRefs] element in [BlkRjct] document" );

	const DOMElement* addRefsNode = dynamic_cast< DOMElement* >( addRefsNodes->item( 0 ) );
	if ( addRefsNode == NULL )
		throw logic_error( "Bad type : [BlkRjct/AddtlRefs[0]] should be an element" );

	DOMNodeList* grpHdrNodes = root->getElementsByTagName( unicodeForm( "GrpHdr" ) );
	if ( ( grpHdrNodes == NULL ) || ( grpHdrNodes->getLength() == 0 ) )
		throw runtime_error( "Missing required [GrpHdr] element in [BlkRjct] document" );

	const DOMElement* grpHdrNode = dynamic_cast< DOMElement* >( grpHdrNodes->item( 0 ) );
	if ( grpHdrNode == NULL )
		throw logic_error( "Bad type : [BlkRjct/GrpHdr] should be an element" );

	//get group id
	{
		DOMNodeList* refNodes = addRefsNode->getElementsByTagName( unicodeForm( "Ref" ) );
		if ( ( refNodes == NULL ) || ( refNodes->getLength() == 0 ) )
			throw runtime_error( "Missing required [Ref] element child of [BlkRjct/AddtlRefs] element" );

		const DOMElement* refNode = dynamic_cast< DOMElement* >( refNodes->item( 0 ) );
		if ( refNode == NULL )
			throw logic_error( "Bad type : [BlkRjct/AddtlRefs[0]/Ref] should be an element" );

		DOMText* ref = dynamic_cast< DOMText* >( refNode->getFirstChild() );
		if ( ref == NULL )
			throw runtime_error( "Missing required [TEXT] element child of [BlkRjct/AddtlRefs/Ref] element" );

		m_GroupId = localForm( ref->getData() );
	}
	//get issuer
	{
		DOMNodeList* refNodes = addRefsNode->getElementsByTagName( unicodeForm( "RefIssr" ) );
		if ( ( refNodes != NULL ) && ( refNodes->getLength() != 0 ) )
		{
			const DOMElement* refNode = dynamic_cast< DOMElement* >( refNodes->item( 0 ) );
			if ( refNode != NULL )
			{
				DOMText* ref = dynamic_cast< DOMText* >( refNode->getFirstChild() );
				if ( ref != NULL )
					m_Issuer = localForm( ref->getData() );
			}
		}
	}
	//get TDFDID
	{
		DOMNodeList* grpIdNodes = grpHdrNode->getElementsByTagName( unicodeForm( "GrpId" ) );
		if ( ( grpIdNodes == NULL ) || ( grpIdNodes->getLength() == 0 ) )
			throw logic_error( "Missing required [GrpId] element child of [BlkRjct/GrpHdr] element" );

		const DOMElement* grpIdNode = dynamic_cast< DOMElement* >( grpIdNodes->item( 0 ) );
		if ( grpIdNode == NULL )
			throw logic_error( "Bad type : [BlkRjct/GrpHdr/GrpId[0]] should be an element" );

		DOMText* grpId = dynamic_cast< DOMText* >( grpIdNode->getFirstChild() );
		if ( grpId == NULL )
			throw runtime_error( "Missing required [TEXT] element child of [BlkRjct/GrpHdr/GrpId] element" );

		m_TFDID = localForm( grpId->getData() );
	}

	DEBUG( "Initialized AchBlkRjctPayload evaluator with groupd id: " << m_GroupId <<\
		" issuer: " << m_Issuer << " TFDID: " << m_TFDID )
}

AchBlkRjctPayload::~AchBlkRjctPayload()
{
	try
	{
		delete m_SequenceResponse;
	}
	catch( ... )
	{
		try
		{
			TRACE( "An error occured while deleting sequence response" );
		} catch( ... ) {}
	}
}

const RoutingAggregationCode& AchBlkRjctPayload::getAggregationCode( const RoutingAggregationCode& feedback )
{
	m_AggregationCode.Clear();

	string correlId = feedback.getCorrelId();
	string correlToken = feedback.getCorrelToken();

	m_AggregationCode.setCorrelToken( correlToken );
	m_AggregationCode.setCorrelId( correlId );

	// request for batch item by trn
	if ( correlToken == RoutingMessageEvaluator::AGGREGATIONTOKEN_TRN )
	{
		wsrm::SequenceFault* response = dynamic_cast< wsrm::SequenceFault* >( AchBlkRjctPayload::getSequenceResponse() );
		if ( response == NULL )
			throw logic_error( "BlkRjctPayload is not a fault sequence" );

		if ( response->IsNack( correlId ) )
		{
			if ( response->getErrorCode() == wsrm::SequenceFault::NOSEQFAULT )
				m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE, response->getNack( correlId ).getErrorCode() );
			else
				m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE, response->getErrorCode() );
		}
		// reactivation code
		else
		{
			m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_FTPCODE, RoutingMessageEvaluator::FEEDBACKFTP_REACT );
		}
		m_AggregationCode.addAggregationCondition( RoutingMessageEvaluator::AGGREGATIONTOKEN_BATCHID, m_GroupId );
		//avoid update already settled messages
		m_AggregationCode.addAggregationCondition( AGGREGATIONTOKEN_TFDCODE + " IS NULL", "" );
	}
	// request for batch
	else if ( correlToken == RoutingMessageEvaluator::AGGREGATIONTOKEN_BATCHID )
	{
		wsrm::SequenceFault* response = dynamic_cast< wsrm::SequenceFault* >( AchBlkRjctPayload::getSequenceResponse() );
		if ( response == NULL )
			throw logic_error( "BlkRjctPayload is not a fault sequence" );

		m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE, response->getErrorCode() );
	}
	// illogic request
	else
	{
		m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_FTPCODE, RoutingMessageEvaluator::FEEDBACKFTP_REACT );
	}

	m_AggregationCode.addAggregationCondition( AGGREGATIONTOKEN_TFDCODE + " IS NULL", "" );
	if ( isDD() || isID() )
	{
		if ( m_Issuer.length() > 0 )
			m_AggregationCode.addAggregationCondition( RoutingMessageEvaluator::AGGREGATIONTOKEN_ISSUER, m_Issuer );
	}

	return m_AggregationCode;
}

string AchBlkRjctPayload::getOutgoingBatchType()
{
	// get batch type
	string batchType = "";
	if ( m_Fields.find( AchBlkRjctPayload::OUTGOINGBATCHTYPE ) == m_Fields.end() )
	{
		if( m_Fields.find( AchBlkRjctPayload::OUTGOINGBATCHTYPE ) != m_Fields.end() )
			throw RoutingExceptionAggregationFailure( "OUTGOINGBATCH", m_GroupId );

		batchType = getBatchType( m_GroupId );
		( void )m_Fields.insert( pair< string, string >( AchBlkRjctPayload::OUTGOINGBATCHTYPE, batchType ) );
	}
	else
		batchType = m_Fields[ AchBlkRjctPayload::OUTGOINGBATCHTYPE ];
	return batchType;
}

string AchBlkRjctPayload::getIncomingBatchType()
{
	string batchType = "";
	if ( m_Fields.find( AchBlkRjctPayload::INCOMINGBATCHTYPE ) == m_Fields.end() )
	{
		if( m_Fields.find( AchBlkRjctPayload::OUTGOINGBATCHTYPE ) != m_Fields.end() )
			throw RoutingExceptionAggregationFailure( "INCOMINGBATCH", m_GroupId );

		batchType = getBatchType( m_GroupId, "BATCHJOBSINC", m_Issuer );
		( void )m_Fields.insert( pair< string, string >( AchBlkRjctPayload::INCOMINGBATCHTYPE, batchType ) );
	}
	else
		batchType = m_Fields[ AchBlkRjctPayload::INCOMINGBATCHTYPE ];
	return batchType;
}

bool AchBlkRjctPayload::isID( const string& batchType )
{
	return ( ( "urn:swift:xs:CoreBlkChq" == batchType ) || ( "urn:swift:xs:CoreBlkPrmsNt" == batchType ) || ( "urn:swift:xs:CoreBlkBillXch" == batchType ) ) ? true : false;
}

bool AchBlkRjctPayload::isID()
{
	bool bisID = false;
	try
	{
		bisID = isID( internalGetBatchType() );
	}
	catch( const RoutingExceptionAggregationFailure& ex2 )
	{
	}
	return bisID;
}

bool AchBlkRjctPayload::isDD()
{
	return ( "urn:swift:xs:CoreBlkLrgRmtDDbt" == internalGetBatchType() );
}

bool AchBlkRjctPayload::isNack()
{
	//History :
	// TFD sends 2 invalid messages for one message sent, we discard one based on matching the status in the grpid
	/*string status = getField( AchBlkRjctPayload::STATUS );
	string tfdId = getField( AchBlkRjctPayload::TFDID );
	return ( tfdId.substr( 0, status.length() ) == status );*/

	// if it was evaluated, its value is bool
	if ( m_NackType != 2 )
		return ( m_NackType == 1 );

	// TFD may send nacks for incoming batches as well... ignore those ( except for dd )
	try
	{
		// get batch type.. if it's there, it's for a sent batch, it's a valid nack
		(void)getOutgoingBatchType();
		m_NackType = 1;
	}
	catch( const RoutingExceptionAggregationFailure& ex )
	{
		m_NackType = 0;

		// look to see if it's a DD
		try
		{
			string batchType = getIncomingBatchType();
			m_NackType = ( isID( batchType ) || ( batchType == "urn:swift:xs:CoreBlkLrgRmtDDbt" ) );
		}
		catch( const RoutingExceptionAggregationFailure& ex2 )
		{
			DEBUG( "Incoming reply for an outside/completed batch" );
		}
	}

	return ( m_NackType == 1 );
}

bool AchBlkRjctPayload::isBatch()
{
	// say it's a batch response ( that needs to be decomposed ) only if it is a real nack
	return isNack();
}

bool AchBlkRjctPayload::isOriginalIncomingMessage()
{
	try
	{
		string batchType = getIncomingBatchType();
		DEBUG( "Incoming batch type = [" << batchType << "]" );
		if ( batchType == "urn:swift:xs:CoreBlkLrgRmtDDbt" )
			return true;
	}
	catch( const RoutingExceptionAggregationFailure& ex )
	{
	}
	return false;
}

bool AchBlkRjctPayload::isRefusal( const string& batchType )
{
	return ( ( batchType == "urn:swift:xs:CoreBlkDDbtRfl" ) || ( batchType == "urn:swift:xs:CoreBlkPrmsNtRfl" ) ||
		( batchType == "urn:swift:xs:CoreBlkChqRfl" ) || ( batchType == "urn:swift:xs:CoreBlkBillXchRfl" ) );
}

bool AchBlkRjctPayload::isRefusal()
{
	bool isRef = false;
	try
	{
		isRef = isRefusal( internalGetBatchType() );
	}
	catch( const RoutingExceptionAggregationFailure& ex2 )
	{
	}
	return isRef;
}

// standards version
wsrm::SequenceResponse* AchBlkRjctPayload::getSequenceResponse()
{
	//XALAN_USING_XALAN( XalanDOMString )

	if ( m_SequenceResponse != NULL )
		return m_SequenceResponse;

	//long batchCount = 0;

	// get all BlkPmtErrHdlg elements
	const DOMElement* root = m_Document->getDocumentElement();
	DOMNodeList* blkPmtErrHdlgNodes = root->getElementsByTagName( unicodeForm( "BlkPmtErrHdlg" ) );

	if ( blkPmtErrHdlgNodes->getLength() == 0 )
		throw runtime_error( "Missing required [BlkPmtErrHdlg] element in [BlkRjct] document" );

	m_SequenceResponse = new wsrm::SequenceFault( m_GroupId );

	const DOMElement* blkPmtErrHdlgNode = NULL;
	DOMNodeList* iteRefNodes = NULL;

	// individual item rejects
	DEBUG( "Some items in batch [" << m_GroupId << "] issued by [" << m_Issuer << "] were rejected. Iterating through individual errors ..." );
	for ( unsigned int i=0; i<blkPmtErrHdlgNodes->getLength(); i++ )
	{
		blkPmtErrHdlgNode = dynamic_cast< DOMElement* >( blkPmtErrHdlgNodes->item( i ) );
		if ( blkPmtErrHdlgNode == NULL )
			throw logic_error( "Bad type : [BlkRjct/BlkPmtErrHdlg] should be an element" );

		DOMNodeList* errCdNodes = blkPmtErrHdlgNode->getElementsByTagName( unicodeForm( "ErrCd" ) );

		if ( ( errCdNodes == NULL ) || ( errCdNodes->getLength() == 0 ) )
			throw runtime_error( "Missing required [ErrCd] element in [BlkRjct/BlkPmtErrHdlg] document" );

		const DOMElement* errCdNode = dynamic_cast< DOMElement* >( errCdNodes->item( 0 ) );
		if ( errCdNode == NULL )
			throw logic_error( "Bad type : [BlkRjct/BlkPmtErrHdlg/ErrCd[0]] should be an element" );

		DOMText* errCd = dynamic_cast< DOMText* >( errCdNode->getFirstChild() );
		if ( errCd == NULL )
			throw runtime_error( "Missing required [TEXT] element child of [BlkRjct/BlkPmtErrHdlg/ErrCd] element" );

		string errorCode = localForm( errCd->getData() );

		iteRefNodes = blkPmtErrHdlgNode->getElementsByTagName( unicodeForm( "IteRef" ) );

		// if we don't have individual item references, set the whole batch as failed
		if ( iteRefNodes->getLength() == 0 )
		{
			DEBUG( "The whole batch [" << m_GroupId << "] issued by [" << m_Issuer << "] was rejected with error code [" << errorCode << "]" );

			wsrm::SequenceFault* response = dynamic_cast< wsrm::SequenceFault* >( m_SequenceResponse );
			if ( response == NULL )
				throw logic_error( "BlkRjctPayload is not a fault sequence" );
			response->setErrorCode( errorCode );

			break;
		}
		else
		{
			const DOMElement* iteRefNode = dynamic_cast< DOMElement* >( iteRefNodes->item( 0 ) );
			if ( iteRefNode == NULL )
				throw logic_error( "Bad type : [BlkRjct/BlkPmtErrHdlg/IteRef] should be an element" );

			DOMText* iteRef = dynamic_cast< DOMText* >( iteRefNode->getFirstChild() );
			if ( iteRef == NULL )
				throw runtime_error( "Missing required [TEXT] element child of [BlkRjct/BlkPmtErrHdlg/IteRef] element" );

			string reference = localForm( iteRef->getData() );
			string originalRef = reference;

			// if it' not for a refusal, get trns form message
			if ( !isRefusal( internalGetBatchType() ) )
			{
				DEBUG( "Rejected item #" << i << " [" << reference << "] with error code [" << errorCode << "]" );
			}
			else
			{
				// obtain original references for refusal
				try
				{
					originalRef = getOriginalRef( reference, m_GroupId );
					if ( originalRef.length() == 0 )
					{
						DEBUG( "Original reference empty... exiting to prevent mass update" );
						throw logic_error( "Original reference empty for refused transaction" );
					}
					DEBUG( "Rejected item #" << i << " [" << originalRef << "] msg. reference [" << reference << "] with error code [" << errorCode << "]" );
				}
				catch( ... )
				{
					TRACE( "Unable to obtain original ref for refusal" );
					throw;
				}
			}

			wsrm::Nack nack( errorCode );
			m_SequenceResponse->AddChild( originalRef, nack );
		}
	}

	//TODO  delete list ?

	DEBUG( "Returning response..." );
	return m_SequenceResponse;
}

bool AchBlkRjctPayload::checkReactivation()
{
	return ( isID() || isRefusal() )? false : true;
}

string AchBlkRjctPayload::internalGetBatchType()
{
	// try to get batch type for sent batch
	try
	{
		return getOutgoingBatchType();
	}
	catch( const RoutingExceptionAggregationFailure& ex )
	{
	}
	// try to get batch type for received batch
	try
	{
		return getIncomingBatchType();
	}
	catch( const RoutingExceptionAggregationFailure& ex )
	{
		return "";
	}
}

string AchBlkRjctPayload::internalToString()
{
	return "";
}

string AchBlkRjctPayload::getMessageType()
{
	return "urn:swift:xs:BlkRjct";
}

