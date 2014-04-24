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

#ifndef ACHBLKRJCTPAYLOAD_H
#define ACHBLKRJCTPAYLOAD_H

#include "WSRM/SequenceResponse.h"

#include "RoutingMessageEvaluator.h"

class AchBlkRjctPayload : public RoutingMessageEvaluator
{
	public:

		AchBlkRjctPayload( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* document );
		~AchBlkRjctPayload();

	private :

		unsigned int m_NackType; // tristate value ( 0-notnack, 1-nack, 2, not evaluated )
		wsrm::SequenceResponse* m_SequenceResponse;

		const static string OUTGOINGBATCHTYPE;
		const static string INCOMINGBATCHTYPE;

		string m_Issuer;
		string m_GroupId;
		string m_TFDID;

		string getOutgoingBatchType();
		string getIncomingBatchType();
		string getMessageType();

	protected :

		string internalGetBatchType();
		string internalToString();

	public :

		const RoutingAggregationCode& getAggregationCode( const RoutingAggregationCode& feedback );

		bool isReply() { return true; }
		bool isAck() { return false; }
		bool isNack();
		bool isNack( const string ns )
		{
			if ( ns.length() == 0 )
				return isNack();
			return ( isNack() && ( ns == internalGetBatchType() ) );
		}
		bool isBatch();

		bool isOriginalIncomingMessage();

		bool isRefusal();
		bool isRefusal( const string& batchType );

		bool isDD();
		bool isID();
		bool isID( const string& batchType );

		string getIssuer()
		{
			return m_Issuer;
		}

		RoutingMessageEvaluator::FeedbackProvider getOverrideFeedbackProvider() { return FEEDBACKPROVIDER_TFD; }

		// standards version
		wsrm::SequenceResponse* getSequenceResponse();
		bool checkReactivation();
};

#endif // ACHBLKRJCTPAYLOAD_H
