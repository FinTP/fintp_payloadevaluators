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

#if ( defined( WIN32 ) && ( _MSC_VER <= 1400 ) )
	#ifdef _HAS_ITERATOR_DEBUGGING
		#undef _HAS_ITERATOR_DEBUGGING
	#endif
	#define _HAS_ITERATOR_DEBUGGING 0
#endif

#include <sstream>
#include <deque>

#include "WSRM/SequenceAcknowledgement.h"
#include "WSRM/SequenceFault.h"

#include "IsoXmlPayload.h"
#include "RoutingEngine.h"
#include "Currency.h"

/*
const string IsoXmlPayload::m_FieldPaths[ ISOXMLPAYLOAD_PATHCOUNT ] = {
	"//x:InstgAgt/x:FinInstnId/x:CmbndId/x:BIC/text()|//x:InstgAgt/x:FinInstnId/x:BIC/text()|//x:DbtrAgt/x:FinInstnId/x:BIC/text()",
	"//x:InstdAgt/x:FinInstnId/x:CmbndId/x:BIC/text()|//x:InstdAgt/x:FinInstnId/x:BIC/text()|//x:CdtrAgt/x:FinInstnId/x:BIC/text()"
};
*/
IsoXmlPayload::IsoXmlPayload( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* document ) : 
	RoutingMessageEvaluator( document, RoutingMessageEvaluator::ISOXML )
{
	m_SequenceResponse = NULL;
}

IsoXmlPayload::~IsoXmlPayload()
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

string IsoXmlPayload::internalToString()
{
	return XmlUtil::SerializeToString( m_Document );
}
/*
string IsoXmlPayload::internalGetField( const int field )
{
	switch( field )
	{
		case InternalXmlPayload::SENDER :
			{
				string messageType = getField( InternalXmlPayload::MESSAGETYPE );
				string senderXPath = "";
				if( messageType == "PmtRtr" || messageType == "pacs.004.001.01" )
					senderXPath = "//x:InstdAgt/x:FinInstnId/x:BIC/text()|//x:CdtrAgt/x:FinInstnId/x:BIC/text()";
				if( messageType == "FIToFIPmtCxlReq" || messageType == "RsltnOfInvstgtn" || messageType == "ReqToModfyPmt" )
					senderXPath = "//x:Assgnr/x:Agt/x:FinInstnId/x:BIC/text()";
				
				if( senderXPath.length() > 0 )
					return XPathHelper::SerializeToString( XPathHelper::Evaluate( senderXPath, m_XalanDocument, m_Namespace ) );
			}
		case InternalXmlPayload::RECEIVER :
			{
				string messageType = getField( InternalXmlPayload::MESSAGETYPE );
				string receiverXPath = "";
				if( messageType == "PmtRtr" || messageType == "pacs.004.001.01" )
					receiverXPath = "//x:InstgAgt/x:FinInstnId/x:BIC/text()|//x:DbtrAgt/x:FinInstnId/x:BIC/text()";
				if( messageType == "FIToFIPmtCxlReq" || messageType == "RsltnOfInvstgtn" || messageType == "ReqToModfyPmt" )
					receiverXPath = "//x:Assgne/x:Agt/x:FinInstnId/x:BIC/text()";


				if( receiverXPath.length() > 0 )
					return XPathHelper::SerializeToString( XPathHelper::Evaluate( receiverXPath, m_XalanDocument, m_Namespace ) );
		
				XALAN_CPP_NAMESPACE_QUALIFIER NodeRefList itemList =  XPathHelper::EvaluateNodes( m_FieldPaths[ field ], m_XalanDocument, m_Namespace );
				for( unsigned index = 0; index < itemList.getLength(); index++ )
				{
					string crtValue = XPathHelper::SerializeToString( itemList.item( index ) );
					if ( crtValue.length() > 0 )
						return crtValue;
				}
				return "";
			}

		case InternalXmlPayload::IOIDENTIFIER :
			return "O";

		case InternalXmlPayload::MESSAGETYPE :
			{
				const DOMElement* root = m_Document->getDocumentElement();
				for ( XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* key = root->getFirstChild(); key != 0; key=key->getNextSibling() )
				{
					if ( key->getNodeType() != DOMNode::ELEMENT_NODE )
						continue;

					string messageType = localForm( key->getLocalName() );
					return messageType;
				}
				return "";
			}

		case InternalXmlPayload::MUR :
			{
				string messageType = getField( InternalXmlPayload::MESSAGETYPE );
				string murXPath = RoutingEngine::GetKeywordXPath( messageType, "MUR" );
				
				if ( murXPath.length() > 0 )
				{
					string rawMur = XPathHelper::SerializeToString( XPathHelper::Evaluate( murXPath, m_XalanDocument, m_Namespace ) );
					return RoutingEngine::EvaluateKeywordValue( messageType, rawMur, "MUR", "value" );
				}
				return "";
			}
			
		case InternalXmlPayload::TRN :
			{
				string messageType = getField( InternalXmlPayload::MESSAGETYPE );
				string trnXPath = RoutingEngine::GetKeywordXPath( messageType, "TRN" );
				return ( trnXPath.length() > 0 ) ? XPathHelper::SerializeToString( XPathHelper::Evaluate( trnXPath, m_XalanDocument, m_Namespace ) ) : "";
			}
		
		case InternalXmlPayload::CURRENCY:
			{
				string messageType = getField( InternalXmlPayload::MESSAGETYPE );
				string currencyXPath = RoutingEngine::GetKeywordXPath( messageType, "Currency" );

				if ( currencyXPath.length() <= 0 )
					return "";

				stringstream result;
				XALAN_CPP_NAMESPACE_QUALIFIER NodeRefList itemList =  XPathHelper::EvaluateNodes( currencyXPath, m_XalanDocument, m_Namespace );
				for( unsigned index = 0; index < itemList.getLength(); index++ )
				{
					result << XPathHelper::SerializeToString( itemList.item( index ) );
				}

				return result.str();
			}

		case InternalXmlPayload::VALUEDATE:
			{
				string messageType = getField( InternalXmlPayload::MESSAGETYPE );
				string vDateXPath = RoutingEngine::GetKeywordXPath( messageType, "ValueDate" );
				string isoDate = ( vDateXPath.length() > 0 ) ? XPathHelper::SerializeToString( XPathHelper::Evaluate( vDateXPath, m_XalanDocument, m_Namespace ) ) : "";
				if ( isoDate.length() == 10 )
					return isoDate.substr( 2, 2 ) + isoDate.substr( 5, 2 ) + isoDate.substr( 8, 2 );
				return isoDate;
			}
			
		case InternalXmlPayload::RELATEDREF :
			{
				string messageType = getField( InternalXmlPayload::MESSAGETYPE );
				string relRefXPath = RoutingEngine::GetKeywordXPath( messageType, "RelatedRef" );
				return ( relRefXPath.length() > 0 ) ? XPathHelper::SerializeToString( XPathHelper::Evaluate( relRefXPath, m_XalanDocument, m_Namespace ) ) : "";
			}

		case InternalXmlPayload::OBATCHID :
			{
				return "";
			}

		case InternalXmlPayload::IBAN :
			{
				if ( RoutingEngine::hasIBANForLiquidities() )
				{
					string messageType = getField( InternalXmlPayload::MESSAGETYPE );
					string ibanXPath = RoutingEngine::GetKeywordXPath( messageType, "IBAN" );
					return ( ibanXPath.length() > 0 ) ? XPathHelper::SerializeToString( XPathHelper::Evaluate( ibanXPath, m_XalanDocument, m_Namespace ) ) : "";
				}
				else
					return "";
			}

		case InternalXmlPayload::IBANPL :
			{
				if ( RoutingEngine::hasIBANPLForLiquidities() )
				{
					string messageType = getField( InternalXmlPayload::MESSAGETYPE );
					string ibanplXPath = RoutingEngine::GetKeywordXPath( messageType, "IBANPL" );
					if ( ibanplXPath.length() > 0 )
					{
						string rawIbanPl = XPathHelper::SerializeToString( XPathHelper::Evaluate( ibanplXPath, m_XalanDocument, m_Namespace ) );
						return RoutingEngine::EvaluateKeywordValue( messageType, rawIbanPl, "IBANPL", "value" );
					}
					return "";
				}
				else
					return "";
			}

		case InternalXmlPayload::SENDERCORR :
			{
				if ( RoutingEngine::hasCorrForLiquidities() )
				{
					string messageType = getField( InternalXmlPayload::MESSAGETYPE );
					string scorrXPath = RoutingEngine::GetKeywordXPath( messageType, "SenderCorresp" );
					if ( scorrXPath.length() > 0 )
					{
						string rawSCorr = XPathHelper::SerializeToString( XPathHelper::Evaluate( scorrXPath, m_XalanDocument, m_Namespace ) );
						return RoutingEngine::EvaluateKeywordValue( messageType, rawSCorr, "SenderCorresp", "value" );
					}
					return "";
				}
				else
					return "";
			}

		case InternalXmlPayload::RECEIVERCORR :
			{
				if ( RoutingEngine::hasCorrForLiquidities() )
				{
					string messageType = getField( InternalXmlPayload::MESSAGETYPE );
					string rcorrXPath = RoutingEngine::GetKeywordXPath( messageType, "ReceiverCorresp" );
					if ( rcorrXPath.length() > 0 )
					{
						string rawRCorr = XPathHelper::SerializeToString( XPathHelper::Evaluate( rcorrXPath, m_XalanDocument, m_Namespace ) );
						return RoutingEngine::EvaluateKeywordValue( messageType, rawRCorr, "ReceiverCorresp", "value" );
					}
					return "";
				}
				else
					return "";
			}

		case InternalXmlPayload::SEQ :
			{
				string messageType = getField( InternalXmlPayload::MESSAGETYPE );
				string seqXPath = RoutingEngine::GetKeywordXPath( messageType, "Sequence" );
				return ( seqXPath.length() > 0 ) ? XPathHelper::SerializeToString( XPathHelper::Evaluate( seqXPath, m_XalanDocument, m_Namespace ) ) : "1";
			}

		case InternalXmlPayload::MAXSEQ :
			{
				string messageType = getField( InternalXmlPayload::MESSAGETYPE );
				string maxSeqXPath = RoutingEngine::GetKeywordXPath( messageType, "MaxSequence" );
				return ( maxSeqXPath.length() > 0 ) ? XPathHelper::SerializeToString( XPathHelper::Evaluate( maxSeqXPath, m_XalanDocument, m_Namespace ) ) : "1";
			}

		case InternalXmlPayload::ORGTXID :
		{
			string messageType = getField( InternalXmlPayload::MESSAGETYPE );
			string orgTxIdXPath = RoutingEngine::GetKeywordXPath( messageType, "ORGTXID" );
			return ( orgTxIdXPath.length() > 0 ) ? XPathHelper::SerializeToString( XPathHelper::Evaluate( orgTxIdXPath, m_XalanDocument, m_Namespace ) ) : "";
		}

		case InternalXmlPayload::ENDTOENDID :
		{
			string messageType = getField( InternalXmlPayload::MESSAGETYPE );
			string endToEndIdXPath = RoutingEngine::GetKeywordXPath( messageType, "ENDTOENDID" );
			return ( endToEndIdXPath.length() > 0 ) ? XPathHelper::SerializeToString( XPathHelper::Evaluate( endToEndIdXPath, m_XalanDocument, m_Namespace ) ) : "";
		}

		case InternalXmlPayload::ORGINSTRID :
		{
			string messageType = getField( InternalXmlPayload::MESSAGETYPE );
			string origInstrIdXPath = RoutingEngine::GetKeywordXPath( messageType, "ORGINSTRID" );
			return ( origInstrIdXPath.length() > 0 ) ? XPathHelper::SerializeToString( XPathHelper::Evaluate( origInstrIdXPath, m_XalanDocument, m_Namespace ) ) : "";
		}

		case InternalXmlPayload::ADDMSGINFO :
		{
			string messageType = getField( InternalXmlPayload::MESSAGETYPE );
			string addInfoXPath = RoutingEngine::GetKeywordXPath( messageType, "MSGINFO" );
			return ( addInfoXPath.length() > 0 ) ? XPathHelper::SerializeToString( XPathHelper::Evaluate( addInfoXPath, m_XalanDocument, m_Namespace ) ) : "";
		}

		default :
			break;
	}
	
	stringstream errorMessage;
	errorMessage << "Unknown field requested [" << field << "]";
	throw invalid_argument( errorMessage.str() );
}
*/


const RoutingAggregationCode& IsoXmlPayload::getAggregationCode( const RoutingAggregationCode& feedback )
{
	return m_AggregationCode;
}

bool IsoXmlPayload::isReply()
{
	return false;
}

bool IsoXmlPayload::isAck()
{
	return false;
}

bool IsoXmlPayload::isNack()
{
	return false;
}

bool IsoXmlPayload::isBatch()
{
	return false;
}

string IsoXmlPayload::getOverrideFeedback()
{
	return "";
}

// standards version
wsrm::SequenceResponse* IsoXmlPayload::getSequenceResponse()
{
	//string batchId = getField( InternalXmlPayload::GROUPID );
	if ( m_SequenceResponse != NULL )
		return m_SequenceResponse;
		
	string messageType = getField( InternalXmlPayload::MESSAGETYPE );

	if ( isAck() )
	{
		m_SequenceResponse = new wsrm::SequenceAcknowledgement( "" );
		
		wsrm::AcknowledgementRange ackrange( 0, 1 );
		m_SequenceResponse->AddChild( ackrange );

		return m_SequenceResponse;
	}
	
	if ( isNack() )
	{
		m_SequenceResponse = new wsrm::SequenceAcknowledgement( "" );
		
		wsrm::AcknowledgementRange nackrange( 0, 1 );
		m_SequenceResponse->AddChild( nackrange );
		
		return m_SequenceResponse;
	}

	return NULL;
}

bool IsoXmlPayload::updateRelatedMessages()
{
	return false;
}

RoutingAggregationCode IsoXmlPayload::getBusinessAggregationCode()
{
	
	RoutingAggregationCode businessReply( RoutingMessageEvaluator::AGGREGATIONTOKEN_TRN, getField( InternalXmlPayload::TRN ) );
	
	return businessReply;
}

/*
void IsoXmlPayload::UpdateMessage( RoutingMessage* message )
{
	string messageType = getField( InternalXmlPayload::MESSAGETYPE );
	DEBUG2( "Updating message of type [" << messageType << "] ... " );
	
	// gsrs set-ups
	if ( StringUtil::StartsWith( messageType, "FI" ) )
	{
		DEBUG( "Setting options for FIxxx messages" );
		
		message->setBatchId( getField( InternalXmlPayload::TRN ) );
		message->setBatchSequence( StringUtil::ParseULong( getField( InternalXmlPayload::SEQ ) ) );
		message->setBatchTotalCount( StringUtil::ParseULong( getField( InternalXmlPayload::MAXSEQ ) ) );

		// generate a message id
		message->setMessageOption( RoutingMessageOptions::MO_GENERATEID );
	}
	
}
*/