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

#ifndef ACHBLKACCPAYLOAD_H
#define ACHBLKACCPAYLOAD_H

#include "WSRM/SequenceResponse.h"

#include "RoutingMessageEvaluator.h"

class AchBlkAccPayload : public RoutingMessageEvaluator
{
	public:

		AchBlkAccPayload( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* document );
		~AchBlkAccPayload();

	private :

		unsigned int m_AckType; // tristate value ( 0-notack, 1-ack, 2, not evaluated )
		wsrm::SequenceResponse* m_SequenceResponse;

		const static string OUTGOINGBATCHTYPE;
		const static string INCOMINGBATCHTYPE;

		string m_OriginalGroupID;
		string m_Issuer;
		string m_Status;

		string getOutgoingBatchType();
		string getIncomingBatchType();

		unsigned int m_IsID;// tristate value ( 0-notack, 1-ack, 2, not evaluated )
		unsigned int m_IsRID;// tristate value ( 0-notack, 1-ack, 2, not evaluated )
		void internalSetDistinctResponses( const DOMNodeList* nodeList );

		string getMessageType();

	protected :

		string internalGetBatchType();
		string internalToString();

	public :

		const RoutingAggregationCode& getAggregationCode( const RoutingAggregationCode& feedback );

		bool isReply(){ return true; }
		bool isAck();
		bool isAck( const string ns )
		{
			if ( ns.length() == 0 )
				return isAck();
			return ( isAck() && ( ns == internalGetBatchType() ) );
		}
		bool isNack() { return false ; }
		bool isBatch();

		bool updateRelatedMessages();
		bool isOriginalIncomingMessage();

		bool isDD();
		bool isID();
		bool isRID();
		bool isRID( const string& batchType );
		bool isID( const string& batchType );

		FinTP::NameValueCollection getAddParams( const string& ref );

		string getIssuer()
		{
			return m_Issuer;
		}

		string getOverrideFeedback();
		RoutingMessageEvaluator::FeedbackProvider getOverrideFeedbackProvider() { return FEEDBACKPROVIDER_TFD; }

		// standards version
		wsrm::SequenceResponse* getSequenceResponse();
		void updateMessage( RoutingMessage* message );
};

#endif // ACHBLKACCPAYLOAD_H
