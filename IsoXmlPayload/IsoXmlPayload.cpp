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

#include "WSRM/SequenceAcknowledgement.h"
#include "WSRM/SequenceFault.h"

#include "IsoXmlPayload.h"
#include "RoutingEngine.h"
#include "Currency.h"

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

string IsoXmlPayload::getMessageType()
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