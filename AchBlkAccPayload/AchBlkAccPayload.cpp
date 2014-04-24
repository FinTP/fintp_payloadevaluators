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

#include "StringUtil.h"

#include "WSRM/SequenceAcknowledgement.h"

#include "AchBlkAccPayload.h"
#include "RoutingMessage.h"
#include "RoutingExceptions.h"

using namespace FinTP;

const string AchBlkAccPayload::OUTGOINGBATCHTYPE = "OBatchType";
const string AchBlkAccPayload::INCOMINGBATCHTYPE = "IBatchType";

AchBlkAccPayload::AchBlkAccPayload( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* document ) :
	RoutingMessageEvaluator( document, RoutingMessageEvaluator::ACHBLKACC ), m_AckType( 2 ), m_IsID( 2 ), m_IsRID( 2 )
{
	m_SequenceResponse = NULL;

	const DOMElement* root = m_Document->getDocumentElement();

	DOMNodeList* addRefsNodes = root->getElementsByTagName( unicodeForm( "AddtlRefs" ) );
	if ( ( addRefsNodes == NULL ) || ( addRefsNodes->getLength() == 0 ) )
		throw runtime_error( "Missing required [AddtlRefs] element in [BlkAcc] document" );

	const DOMElement* addRefsNode = dynamic_cast< DOMElement* >( addRefsNodes->item( 0 ) );
	if ( addRefsNode == NULL )
		throw logic_error( "Bad type : [BlkAcc/AddtlRefs[0]] should be an element" );

	//get original group id
	{
		DOMNodeList* refNodes = addRefsNode->getElementsByTagName( unicodeForm( "Ref" ) );
		if ( ( refNodes == NULL ) || ( refNodes->getLength() == 0 ) )
			throw runtime_error( "Missing required [Ref] element child of [BlkAcc/AddtlRefs] element" );

		const DOMElement* refNode = dynamic_cast< DOMElement* >( refNodes->item( 0 ) );
		if ( refNode == NULL )
			throw logic_error( "Bad type : [BlkAcc/AddtlRefs[0]/Ref] should be an element" );

		DOMText* ref = dynamic_cast< DOMText* >( refNode->getFirstChild() );
		if ( ref == NULL )
			throw runtime_error( "Missing required [TEXT] element child of [BlkAcc/AddtlRefs/Ref] element" );

		m_OriginalGroupID = XmlUtil::XMLChtoString( ref->getData() );
	}
	//get issuer
	{
		DOMNodeList* refNodes = addRefsNode->getElementsByTagName( unicodeForm( "RefIssr" ) );
		if ( ( refNodes == NULL ) || ( refNodes->getLength() == 0 ) )
			m_Issuer = "";
		else
		{
			const DOMElement* refNode = dynamic_cast< DOMElement* >( refNodes->item( 0 ) );
			if ( refNode != NULL )
			{
				DOMText* ref = dynamic_cast< DOMText* >( refNode->getFirstChild() );
				if ( ref != NULL )
					m_Issuer = XmlUtil::XMLChtoString( ref->getData() );
			}
		}
	}
	//get status
	{
		DOMNodeList* grpHdrNodes = root->getElementsByTagName( unicodeForm( "GrpHdr" ) );
		if ( ( grpHdrNodes == NULL ) || ( grpHdrNodes->getLength() == 0 ) )
			throw runtime_error( "Missing required [GrpHdr] element in [BlkAcc] document" );

		const DOMElement* grpHdrNode = dynamic_cast< DOMElement* >( grpHdrNodes->item( 0 ) );
		if ( grpHdrNode == NULL )
			throw logic_error( "Bad type : [BlkAcc/GrpHdr[0]] should be an element" );

		DOMNodeList* sttsNodes = grpHdrNode->getElementsByTagName( unicodeForm( "Stts" ) );
		if ( ( sttsNodes == NULL ) || ( sttsNodes->getLength() == 0 ) )
			throw runtime_error( "Missing required [Stts] element child of [BlkAcc/GrpHdr] element" );

		const DOMElement* sttsNode = dynamic_cast< DOMElement* >( sttsNodes->item( 0 ) );
		if ( sttsNode == NULL )
			throw logic_error( "Bad type : [BlkAcc/GrpHdr[0]/Stts] should be an element" );

		DOMText* stts = dynamic_cast< DOMText* >( sttsNode->getFirstChild() );
		if ( stts == NULL )
			throw runtime_error( "Missing required [TEXT] element child of [BlkAcc/GrpHdr/Stts] element" );

		m_Status = XmlUtil::XMLChtoString( stts->getData() );
	}

	DEBUG( "Initialized AchBlkAccPayload evaluator with groupd id: " << m_OriginalGroupID <<\
		" issuer: " << m_Issuer << " status: " << m_Status )
}

AchBlkAccPayload::~AchBlkAccPayload()
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

const RoutingAggregationCode& AchBlkAccPayload::getAggregationCode( const RoutingAggregationCode& feedback )
{
	string correlId = feedback.getCorrelId();
	string correlToken = feedback.getCorrelToken();
	m_AggregationCode.Clear();
	m_AggregationCode.setCorrelToken( correlToken );
	m_AggregationCode.setCorrelId( correlId );

	//	m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE, RoutingMessageEvaluator::FEEDBACKFTP_ACK );

	// request for batch settled item by trn
	if ( correlToken == RoutingMessageEvaluator::AGGREGATIONTOKEN_TRN )
	{
		wsrm::SequenceAcknowledgement* response = dynamic_cast< wsrm::SequenceAcknowledgement* >( AchBlkAccPayload::getSequenceResponse() );
		if ( response == NULL )
			throw logic_error( "Sequence response is not ack sequence" );

		if ( response->IsAck( correlId ) )
		{
			m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE, RoutingMessageEvaluator::FEEDBACKFTP_ACK );
		}
		else
		{
			m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE, RoutingMessageEvaluator::FEEDBACKFTP_LOSTRFD );
			//avoid update messages with the same TRN
			m_AggregationCode.addAggregationCondition( RoutingMessageEvaluator::AGGREGATIONTOKEN_BATCHID, m_OriginalGroupID );
		}
		//avoid update already totaly refused or rejected messages
		m_AggregationCode.addAggregationCondition( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE +" IS NULL", "" );
	}
	else
	{
		if ( ( m_Status == "SETTLED" ) || ( !isID() && !isRID() && ( m_MarkACHApproved && ( m_Status != "APPROVED" ) ) ) )
			m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE, feedback.getAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE ) );
		else
			m_AggregationCode.addAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_SAACODE, feedback.getAggregationField( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE ) );

		//try
		//{
		// for a sent batch add a condition : TFDCODE is NULL so that we don't return rejected messages
		//string batchType = getOutgoingBatchType();
		m_AggregationCode.addAggregationCondition( RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE + " IS NULL", "" );

		if ( isDD() || isID() )
		{
			if ( m_Issuer.length() > 0 )
				m_AggregationCode.addAggregationCondition( RoutingMessageEvaluator::AGGREGATIONTOKEN_ISSUER, m_Issuer );
		}
		//}
		//catch( const RoutingExceptionAggregationFailure& ex )
		//{
		// Do nothing... the condition should be TFDCODE == 'QPI12'
		//}
	}
	return m_AggregationCode;
}

string AchBlkAccPayload::getOutgoingBatchType()
{
	// get batch type
	string batchType = "";
	if ( m_Fields.find( OUTGOINGBATCHTYPE ) == m_Fields.end() )
	{
		if( m_Fields.find( INCOMINGBATCHTYPE ) != m_Fields.end() )
			throw RoutingExceptionAggregationFailure( "OUTGOINGBATCH", m_OriginalGroupID );

		batchType = getBatchType( m_OriginalGroupID );
		( void )m_Fields.insert( pair< string, string >( OUTGOINGBATCHTYPE, batchType ) );
	}
	else
		batchType = m_Fields[ OUTGOINGBATCHTYPE ];
	return batchType;
}

string AchBlkAccPayload::getIncomingBatchType()
{
	string batchType = "";
	if ( m_Fields.find( INCOMINGBATCHTYPE ) == m_Fields.end() )
	{
		if( m_Fields.find( OUTGOINGBATCHTYPE ) != m_Fields.end() )
			throw RoutingExceptionAggregationFailure( "INCOMINGBATCH", m_OriginalGroupID );

		batchType = getBatchType( m_OriginalGroupID, "BATCHJOBSINC", m_Issuer );
		( void )m_Fields.insert( pair< string, string >( INCOMINGBATCHTYPE, batchType ) );
	}
	else
		batchType = m_Fields[ INCOMINGBATCHTYPE ];
	return batchType;
}

bool AchBlkAccPayload::isDD()
{
	return ( "urn:swift:xs:CoreBlkLrgRmtDDbt" == internalGetBatchType() );
}

bool AchBlkAccPayload::isID( const string& batchType )
{
	return ( "urn:swift:xs:CoreBlkChq" == batchType ) || ( "urn:swift:xs:CoreBlkPrmsNt" == batchType ) || ( "urn:swift:xs:CoreBlkBillXch" == batchType );
}

bool AchBlkAccPayload::isID()
{
	if ( m_IsID != 2 )
		return ( m_IsID == 1 );

	try
	{
		m_IsID = isID( internalGetBatchType() );
	}
	catch( const RoutingExceptionAggregationFailure& )
	{
		m_IsID = 0;
	}

	return ( m_IsID == 1 );
}

bool AchBlkAccPayload::isRID( const string& batchType )
{
	return ( ( "urn:swift:xs:CoreBlkPrmsNtRfl" == batchType ) || ( "urn:swift:xs:CoreBlkChqRfl" == batchType ) || ( "urn:swift:xs:CoreBlkBillXchRfl" == batchType ) ) ? true : false;
}

bool AchBlkAccPayload::isRID()
{
	if ( m_IsRID != 2 )
		return ( m_IsRID == 1 );

	try
	{
		m_IsRID = isRID( internalGetBatchType() );
	}
	catch( const RoutingExceptionAggregationFailure& )
	{
		m_IsRID = 0;
	}

	return ( m_IsRID == 1 );
}

bool AchBlkAccPayload::isAck()
{
	// if it was evaluated, its value is bool
	if ( m_AckType != 2 )
		return ( m_AckType == 1 );

	//added APPROVED as ack for DD refusal
	if ( ( m_Status != "SETTLED" ) && ( m_Status != "APPROVED" ) )
	{
		m_AckType = 0;
		return false;
	}

	// TFD sends this acks for incoming batches as well... ignore those
	// this will return true if there are records matching the add code

	try
	{
		// get batch type

		// for 104R return is ack in any case ( SETTLED or APPROVED )
		// for all other messages return ack only if SETTLED for a sent batch
		if ( ( internalGetBatchType() == "urn:swift:xs:CoreBlkDDbtRfl" ) || isID() || isRID() )
			m_AckType = 1;
		else
			m_AckType = ( m_Status == "SETTLED" );
	}
	catch( const RoutingExceptionAggregationFailure& )
	{
		m_AckType = 0;
		if ( m_Status == "SETTLED" )
		{
			// SETTLED as ack for /IDDD incoming messages
			try
			{
				string batchType = getIncomingBatchType();
				m_AckType = ( isID( batchType ) || ( batchType == "urn:swift:xs:CoreBlkLrgRmtDDbt" ) );
			}
			catch( const RoutingExceptionAggregationFailure& )
			{
				DEBUG( "Incoming reply for an outside/completed batch" );
			}
		}
	}

	return ( m_AckType == 1 );
}

bool AchBlkAccPayload::isBatch()
{
	// say it's a batch response ( that needs to be decomposed ) only if it is a real ack or
	//.config says approved batch response is batch response too
	return ( isAck() || ( m_MarkACHApproved && isReply() && ( m_Status == "APPROVED" ) ) );
}

bool AchBlkAccPayload::updateRelatedMessages()
{
	try
	{
		// update refused messages in feedbackagg when the refusal or rid  status is approved
		string batchType = getOutgoingBatchType();
		if ( ( ( batchType == "urn:swift:xs:CoreBlkDDbtRfl" ) || isRID( batchType ) ) && ( m_Status == "APPROVED" ) )
			return true;
	}
	catch( const RoutingExceptionAggregationFailure )
	{
	}
	return false;
}

void AchBlkAccPayload::updateMessage( RoutingMessage* message )
{
	string outgoingType = getOutgoingBatchType();
	if ( isID() && ( outgoingType.length() > 0 ) )
		message->setMessageOptions( RoutingMessageOptions::MO_MARKNOTREPLIED );
}

bool AchBlkAccPayload::isOriginalIncomingMessage()
{
	try
	{
		string batchType = getIncomingBatchType();
		if ( batchType == "urn:swift:xs:CoreBlkLrgRmtDDbt" )
			return true;
	}
	catch( const RoutingExceptionAggregationFailure& ex )
	{
	}
	return false;
}

string AchBlkAccPayload::getOverrideFeedback()
{
	// override feedback is used to update codes in FEEDBACKAGG for batches.
	// only executed whend sending batch replies and APPROVED will not be considered ack ( except for rfl )
	if ( m_Status == "APPROVED" )
	{
		return ( isID() || isRID() || m_MarkACHApproved ) ? RoutingMessageEvaluator::FEEDBACKFTP_APPROVED : RoutingMessageEvaluator::FEEDBACKFTP_ACK;
	}
	if ( m_Status == "SETTLED" )
		return RoutingMessageEvaluator::FEEDBACKFTP_ACK;

	return m_Status;
}

// standards version
wsrm::SequenceResponse* AchBlkAccPayload::getSequenceResponse()
{
	if ( m_SequenceResponse != NULL )
		return m_SequenceResponse;

	// get all settlement details elements
	const DOMElement* root = m_Document->getDocumentElement();
	string outgoingType = "";
	//treat BulkAck reply for incomming ID batches like always
	//consider BulkAck reply transactions for outgoing ID batches
	try
	{
		outgoingType = getOutgoingBatchType();
	}
	catch( const RoutingExceptionAggregationFailure& ex )
	{
	}
	if ( isID() && ( outgoingType.length() > 0 ) )
	{
		DOMNodeList* blkSttlmDtlsNodes = root->getElementsByTagName( unicodeForm( "SttlmDtls" ) );
		if ( blkSttlmDtlsNodes->getLength() > 0 )
		{
			DEBUG( "Some items in batch [" << m_OriginalGroupID << "] issued by [" << m_Issuer << "] were settled. Iterating through individual settlements ..." );
			m_SequenceResponse = new wsrm::SequenceAcknowledgement( m_OriginalGroupID );
			internalSetDistinctResponses( blkSttlmDtlsNodes );
			DEBUG( "Returning response..." );

		}
		else
		{
			m_SequenceResponse = new wsrm::SequenceAcknowledgement( m_OriginalGroupID );
			wsrm::AcknowledgementRange ackrange( 1, 0 );
			m_SequenceResponse->AddChild( ackrange );
		}
	}
	else
	{
		m_SequenceResponse = new wsrm::SequenceAcknowledgement( m_OriginalGroupID );
		wsrm::AcknowledgementRange ackrange( 1, 0 );
		m_SequenceResponse->AddChild( ackrange );
	}
	return m_SequenceResponse;
}

NameValueCollection AchBlkAccPayload::getAddParams( const string& ref )
{
	NameValueCollection addParams;
	//HACK isDD should not be here, but all incoming batches are marked as DD
	if ( isID() || isDD() )
	{
		DEBUG( "Tring to find batch item with ref [" << ref << "] in BlkAcc" );

		// get all BlkPmtErrHdlg elements
		const DOMElement* root = m_Document->getDocumentElement();
		DOMNodeList* sttlmDtlsNodes = root->getElementsByTagName( unicodeForm( "SttlmDtls" ) );

		if ( sttlmDtlsNodes->getLength() == 0 )
		{
			TRACE( "Missing required [SttlmDtls] element in [BlkAcc] document" );
			return addParams;
		}

		// individual item rejects
		for ( unsigned int i=0; i<sttlmDtlsNodes->getLength(); i++ )
		{
			const DOMElement* sttlmDtlsNode = dynamic_cast< DOMElement* >( sttlmDtlsNodes->item( i ) );
			if ( sttlmDtlsNode == NULL )
				throw logic_error( "Bad type : [BlkAcc/SttlmDtls] should be an element" );

			DOMNodeList* iteRefNodes = sttlmDtlsNode->getElementsByTagName( unicodeForm( "IteRef" ) );

			if ( ( iteRefNodes == NULL ) || ( iteRefNodes->getLength() == 0 ) )
				throw runtime_error( "Missing required [IteRef] element in [BlkAcc/SttlmDtls] document" );

			const DOMElement* iteRefNode = dynamic_cast< DOMElement* >( iteRefNodes->item( 0 ) );
			if ( iteRefNode == NULL )
				throw logic_error( "Bad type : [BlkAcc/SttlmDtls/IteRef[0]] should be an element" );

			DOMText* iteRef = dynamic_cast< DOMText* >( iteRefNode->getFirstChild() );
			if ( iteRef == NULL )
				throw runtime_error( "Missing required [TEXT] element child of [BlkAcc/SttlmDtls/IteRef[0]] element" );

			string iteRefText = localForm( iteRef->getData() );
			if( ref != iteRefText )
				continue;

			DOMNodeList* sttldAmtNodes = sttlmDtlsNode->getElementsByTagName( unicodeForm( "SttldAmt" ) );

			// if we don't have individual amount, don't add it to params
			if ( sttldAmtNodes->getLength() == 0 )
			{
				DEBUG( "Item [" << ref << "] doesn't have SttldAmt." );
			}
			else
			{
				const DOMElement* sttldAmtNode = dynamic_cast< DOMElement* >( sttldAmtNodes->item( 0 ) );
				if ( sttldAmtNode == NULL )
					throw logic_error( "Bad type : [BlkAcc/SttlmDtls/SttldAmt] should be an element" );

				DOMText* iteAmount = dynamic_cast< DOMText* >( sttldAmtNode->getFirstChild() );
				if ( iteAmount == NULL )
					throw runtime_error( "Missing required [TEXT] element child of [BlkAcc/SttlmDtls/SttldAmt] element" );

				string amount = localForm( iteAmount->getData() );
				DEBUG( "Item [" << ref << "] has SttldAmt [" << amount << "]" );

				addParams.Add( "XSLTPARAMSTTLDAMT", StringUtil::Pad( amount, "\'", "\'" ) );
			}

			break;
		}
	}
	return addParams;
}

void AchBlkAccPayload::internalSetDistinctResponses( const DOMNodeList* blkSttlmDtlsNodes  )
{
	string sttldAmt = "";

	for ( unsigned int i=0; i<blkSttlmDtlsNodes->getLength(); i++ )
	{
		const DOMElement* blkSttlmDtlsNode = dynamic_cast< DOMElement* >( blkSttlmDtlsNodes->item( i ) );
		if ( blkSttlmDtlsNode == NULL )
			throw logic_error( "Bad type : [BlkAcc/SttlmDtls] should be an element" );

		DOMNodeList* refNodes = blkSttlmDtlsNode->getElementsByTagName( unicodeForm( "IteRef" ) );

		if ( ( refNodes == NULL ) || ( refNodes->getLength() == 0 ) )
			throw runtime_error( "Missing required [IteRef] element of [BlkAcc/SttlmDtls] element" );

		const DOMElement* refNode = dynamic_cast< DOMElement* >( refNodes->item( 0 ) );
		if ( refNode == NULL )
			throw logic_error( "Bad type : [BlkAcc/SttlmDtls/IteRef[0]] should be an element" );

		DOMText* iteRef = dynamic_cast< DOMText* >( refNode->getFirstChild() );
		if ( iteRef == NULL )
			throw runtime_error( "Missing required [TEXT] element child of [BlkAcc/SttlmDtls/IteRef] element" );

		string originalRef = localForm( iteRef->getData() );

		wsrm::Ack ack ;
		m_SequenceResponse->AddChild( originalRef, ack );
	}
}

string AchBlkAccPayload::internalGetBatchType()
{
	try
	{
		// try to get batch type for sent batch
		return getOutgoingBatchType();
	}
	catch( const RoutingExceptionAggregationFailure& )
	{
		// try to get batch type for received batch
		try
		{
			return getIncomingBatchType();
		}
		catch( RoutingExceptionAggregationFailure& )
		{
			return "";
		}
	}
}

string AchBlkAccPayload::internalToString()
{
	return "";
}

string AchBlkAccPayload::getMessageType()
{
	return "urn:swift:xs:BlkAcc";
}

