//
// Copyright (c) 2012 Regents of the SIGNET lab, University of Padova.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the University of Padova (SIGNET lab) nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/**
 * @file   uwtp-module.cc
 * @author Saiful Azad
 * @version 1.0.0
 *
 * @brief  Implementations of a simple Transport Layer Protocol (UWTP)
 */

#include <cassert>
#include <iomanip>
#include <iostream>

#include <uwip-module.h>

#include "uwtp-module.h"

/**
 * Class that represent the binding with tcl scripting language
 */
static class UWTPClass : public TclClass
{
public:
	/**
	 * Constructor of the class
	 */
	UWTPClass()
		: TclClass("Module/UW/TP")
	{
	}

	TclObject *
	create(int, const char *const *)
	{
		return (new UWTP);
	}
} class_module_uwtp;

void
UWTP_delayTimer::expire(Event *e)
{

	module->sendNack();
}

UWTP::UWTP()
	: portcounter(0)
	, ack_tx_count(0)
	, nack_tx_count(0)
	, ack_rx_count(0)
	, nack_rx_count(0)
	, delay_timer_(this)
	, ack_mode(WITH_ACK)
	, destPort_(0)
	, seq_no_counter(-1)
	, ack_tx_mode(WITHOUT_CUM_ACK)
{
	if (portcounter != 0)
		portcounter = 0;

	bind("debug_", &debug_);
	bind("send_buffer_size_", (int *) &send_buffer_size);
	bind("receive_buffer_size_", (int *) &receive_buffer_size);
	bind("delay_interval_", (double *) &delay_interval);
	bind("destPort_", (int *) &destPort_);
	bind("nack_retx_time_", (double *) &nack_retx_time);
	bind("pkt_delete_time_from_queue_", (double *) &pkt_delete_time_from_queue);
	bind("expected_ACK_threshold_", (double *) &expected_ACK_threshold);
	bind("cum_ACK_param_", (int *) &cum_ack_parameter);
	bind("nack_retx_limit_", (int *) &nack_retx_limit);
	cout << "PARAMETRI\n" << send_buffer_size << " " << receive_buffer_size << ", ack param = " << cum_ack_parameter << endl << "\nnack retx limit: " << nack_retx_limit << endl;
}

UWTP::~UWTP()
{
}

int
UWTP::command(int argc, const char *const *argv)
{
	Tcl &tcl = Tcl::instance();
	if (argc == 2) {
		if (strcasecmp(argv[1], "getUWTPDataHSize") == 0) {
			tcl.resultf("%d", getUWTPDataHSize());
			return TCL_OK;
		} else if (strcasecmp(argv[1], "getUWTPAckHSize") == 0) {
			tcl.resultf("%d", getUWTPAckHSize());
			return TCL_OK;
		} else if (strcasecmp(argv[1], "getUWTPNackHSize") == 0) {
			tcl.resultf("%d", getUWTPNackHSize());
			return TCL_OK;
		} else if (strcasecmp(argv[1], "getAckTxCount") == 0) {
			tcl.resultf("%d", ack_tx_count);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "getNackTxCount") == 0) {
			tcl.resultf("%d", nack_tx_count);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "getAckRxCount") == 0) {
			tcl.resultf("%d", ack_rx_count);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "getNackRxCount") == 0) {
			tcl.resultf("%d", nack_rx_count);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "setAckMode") == 0) {
			ack_mode = WITH_ACK;
			return TCL_OK;
		} else if (strcasecmp(argv[1], "setNoAckMode") == 0) {
			ack_mode = WITHOUT_ACK;
			return TCL_OK;
		} else if (strcasecmp(argv[1], "setCumAckMode") == 0) {
			ack_tx_mode = WITH_CUM_ACK;
			return TCL_OK;
		} else if (strcasecmp(argv[1], "setNoCumAckMode") == 0) {
			ack_tx_mode = WITHOUT_CUM_ACK;
			return TCL_OK;
		}
	}

	if (argc == 3) {
		if (strcasecmp(argv[1], "assignPort") == 0) {
			Module *m = dynamic_cast<Module *>(tcl.lookup(argv[2]));
			if (!m)
				return TCL_ERROR;
			int port = assignPort(m);
			tcl.resultf("%d", port);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "node_id") == 0) {
			node_id_ = atoi(argv[2]);
			return TCL_OK;
		}
	}
	return Module::command(argc, argv);
}

void
UWTP::recv(Packet *p)
{
	cout << TIME << " UWTP::recv(" << node_id
		 << "), a Packet is sent without source module!!" << endl;
	Packet::free(p);
}

void
UWTP::initPkt(Packet *p, int id, int seq_no_counter)
{

	//create the packet header
	struct hdr_cmn *cmh = HDR_CMN(p);
	struct hdr_uwtp_data *uwtpdh = HDR_UWTP_DATA(p);

	if (debug_) cout << "chiamata a initPkt, seqNum = " << seq_no_counter << endl;
		//cout << TIME << " UWTP::initPkt(" << node_id
		//	 << "), initializing data packet " << endl;

	map<int, int>::const_iterator iter = port_map.find(id);

	if (iter == port_map.end()) {
		fprintf(stderr,
				"UWTP::recvData() no port assigned to id %d, dropping "
				"packet!!!\n",
				id);
		Packet::free(p);
	}

	int sport = iter->second;
	assert(sport > 0 && sport <= portcounter);

	//setup the packet fields
	cmh->direction() = hdr_cmn::DOWN;
	uwtpdh->type_ = UWTP_DATA;
	cmh->uid() = seq_no_counter;  //uses the uid to store the sequence number
	uwtpdh->sport_ = sport;
	uwtpdh->dport_ = destPort_;
	cmh->size() += uwtpdh->size();
	if (debug_) {
		cout << "INVIO: da " << getId() << ", SN = " << cmh->uid() << endl;
	//	cout << "Packet type " << uwtpdh->type_ << endl;
	//	cout << "Seq no " << cmh->uid() << endl;
	//	cout << "Source port " << uwtpdh->sport_ << endl;
	//	cout << "Destination port " << uwtpdh->dport_ << endl;
	//	cout << "\n" << endl;
	}
}

void
UWTP::initAckPkt(Packet *p, Packet *ack_pkt, int seq_no)
{

	if (debug_) cout << "initACK seqNum = " << seq_no << endl;
		//cout << TIME << " UWTP::initACKPkt(" << node_id
			 //<< "), initializing ACK packet" << endl;


	//create the complete packet with also ip header
	struct hdr_cmn *cmh = HDR_CMN(p);
	struct hdr_uwtp_data *uwtpdh = HDR_UWTP_DATA(p);
	struct hdr_uwip *iph = HDR_UWIP(p);
	struct hdr_cmn *cmha = HDR_CMN(ack_pkt);
	struct hdr_uwtp_ack *uwtpah = HDR_UWTP_ACK(ack_pkt);
	struct hdr_uwip *ipah = HDR_UWIP(ack_pkt);

	ipah->daddr() = iph->saddr();
	uwtpah->type_ = UWTP_ACK;
	uwtpah->sport_ = uwtpdh->getDport();
	uwtpah->dport_ = uwtpdh->getSport();
	uwtpah->seq_no_ = seq_no;	//set the sequence number
	cmha->direction() = hdr_cmn::DOWN;
	cmha->size() += uwtpah->size();
}

void
UWTP::initNackPkt(Packet *p, Packet *nack_pkt, int nack_seq_no)
{

	if (debug_) cout << "initNACK, seqNum = " << nack_seq_no << endl;
		//cout << TIME << " UWTP::initNackPkt(" << node_id
		//	 << "), initializing NACK packet " << endl
		//	 << "\nNACK seqNum = " << nack_seq_no << endl << endl;

	//setup the headers
	struct hdr_cmn *cmh = HDR_CMN(p);
	struct hdr_uwtp_data *uwtpdh = HDR_UWTP_DATA(p);
	struct hdr_uwip *iph = HDR_UWIP(p);
	struct hdr_cmn *cmhna = HDR_CMN(nack_pkt);
	struct hdr_uwtp_nack *uwtpnah = HDR_UWTP_NACK(nack_pkt);
	struct hdr_uwip *ipnah = HDR_UWIP(nack_pkt);

	ipnah->daddr() = iph->saddr();
	uwtpnah->type_ = UWTP_NACK;
	uwtpnah->sport_ = uwtpdh->getDport();
	uwtpnah->dport_ = uwtpdh->getSport();
	uwtpnah->seq_no_ = nack_seq_no;
	cmhna->direction() = hdr_cmn::DOWN;
	cmhna->size() += uwtpnah->size();
}

void
UWTP::sendAck(Packet *p)
{

	if (debug_) cout << "sendACK" << endl;
		//cout << TIME << " UWTP::sendAck(" << node_id << "), sending Ack "
		//	 << endl;

	ack_tx_count++;

	hdr_uwtp_ack *uah = HDR_UWTP_ACK(p);  //why is this here

	if (p != NULL) {	//if the packet exists send it down
		sendDown(p);
	} else
		cout << TIME << "Program is pointing to a null pointer " << endl;
}

void
UWTP::sendNack()
{

	if (debug_) cout << "\n\n===========================\n" << TIME << "   -------- sendNACK ---------" << endl;
		//cout << TIME << " UWTP::sendNack(" << node_id << "), sending Nack "
		//	 << endl;

	if (debug_) {
		cout << "Now printing the content of the NACK buffer" << endl;
		for (map<UWTPPair, NackPktStoreInfo *>::iterator it_nb =
						nackBuffer.begin();
				it_nb != nackBuffer.end();
				it_nb++) {
			cout << TIME << " Destination port : " << (it_nb->first).first
				 << " Seq no " << (it_nb->first).second << " nack tx info "
				 << it_nb->second->getNackTxInfo() << endl;
		}
	}


	//before doing anything, clean nacks that cant be retxed
	//need to make another cycle because I didnt wanna risk undefined behaviour in the previous one when removing iterators
	for (map<UWTPPair, NackPktStoreInfo *>::iterator it_nb =
					nackBuffer.begin();
			it_nb != nackBuffer.end();
			){
		if(it_nb->second->getRetxNum() >= nack_retx_limit){	//if the nack has reached the max number of retransmissions
			map<UWTPPair, NackPktStoreInfo *>::iterator toErase = it_nb;

			int seq_no = (it_nb->first).second;
			int dport_no = (it_nb->first).first;
			int sender_id = it_nb->second->getSenderId();
			if(debug_) cout << "\n\n\n\n+++++++++++++++++++++++++\nLimit of retransmissions for NACK port:" << dport_no << ", sn:" << seq_no << " was reached.\nStop trying" << endl;


			++it_nb;
			nackBuffer.erase(toErase);	//remove the nack from the buffer

			map<PortNo, ExpectedPktSeqNo>::iterator it_e = expPktInfo.find(dport_no);
			it_e->second += 1;			//update the next expected sequence number, like if the packet was received

			checkReceiveQueue(dport_no, sender_id);	
			

		}
		else{
			++it_nb;
		}
	}

	

	//this part is necessary because the user does not need to specify a nack to send, but
	//just issues the send of a nack, if there is one in the queue
	if (nackBuffer.size() != 0) {
		
		for (map<UWTPPair, NackPktStoreInfo *>::iterator it_nb =
						nackBuffer.begin();
				it_nb != nackBuffer.end();
				it_nb++) {			//iterate over all packets in the buffer

			if (it_nb->second->getNackTxInfo() == FALSE) {  //if the packet has not been transmitted yet

				it_nb->second->setNackTxInfo(TRUE);  		//flag it as transmitted
				it_nb->second->setNackTxTime(TIME);			//set the time of transmission
				Packet *nack_p = (it_nb->second->getNackPnt())->copy();
				it_nb->second->countRetx();
				nack_tx_count++;
				sendDown(nack_p);
				
			} else if (it_nb->second->getNackTxInfo() == TRUE &&
					it_nb->second->getTimeSpentInNackBuffer() >
							nack_retx_time) {				//else if the packet has been flagged for transmission
															//and the time needed to retransmission has passed
				it_nb->second->setNackTxTime(TIME);			//update the time	
				
				it_nb->second->countRetx();
				Packet *nack_p = (it_nb->second->getNackPnt())->copy();
				nack_tx_count++;
				sendDown(nack_p);							//resend the packet
				

			} else {
				// do nothing
			}
		}

	}


	//if after the procedure there are still nacks in the buffer
	if (nackBuffer.size() != 0) {

		/* I honestly dont think this does anything useful 
		because if there is something in the buffer we will eventually need to send it 
		or it will be removed anyway


		int count;
		for (map<UWTPPair, NackPktStoreInfo *>::iterator it_nb =
						nackBuffer.begin();
				it_nb != nackBuffer.end();
				it_nb++) {			//iterate over all packets in the buffer
			if (it_nb->second->getNackTxInfo() == FALSE ||		
					it_nb->second->getTimeSpentInNackBuffer() > nack_retx_time)
				count++;			//count all nacks in the buffer that have not been transmitted yet
									//or that should be retransmitted 
									//which means count packets we have done something with
									//which means we could have counted them in the previous section??
		}

		if (count >= 0) {										//if before we did something
			//delay_timer_.resched(JITTER * delay_interval);		//idk reschedule the timer for the future or smth
			delay_timer_.resched(delay_interval);
			if(debug_) cout << "there is stuff: reschedule" << endl;
		}
		*/

		delay_timer_.resched(delay_interval);
		if(debug_) cout << "there is stuff: reschedule" << endl;
	}
	else {
		if(debug_) cout << "nothing to send" << endl;
	}
}


//check receive queue for packets to send to the upper layer
void
UWTP::checkReceiveQueue(int port, int id)
{

	if (debug_) cout << "checkReceiveQueue; queue size for port " << port << " is: " << receiveBuffer.size(	) << endl;
		//cout << TIME << " UWTP::checkReceiveQueue(" << node_id
		//	 << "), any other packet which can be possible to transfer to the "
		//		"upper layer"
		//	 << endl;

	if (debug_){
		cout << "Content is: " << endl;
		for (map<UWTPPair, Packet *>::iterator it_r = receiveBuffer.begin(); //UWTPPair = portNum + seqNum
			it_r != receiveBuffer.end();
			it_r++) {
				cout << "port = " << (it_r->first).first << ", SN = " << (it_r->first).second << endl;
			}

	}

	map<PortNo, ExpectedPktSeqNo>::iterator it_e = expPktInfo.find(port);

	for (map<UWTPPair, Packet *>::iterator it_r = receiveBuffer.begin(); //UWTPPair = portNum + seqNum
			it_r != receiveBuffer.end();
			it_r++) {				//iterate over the receive buffer
		if ((it_r->first).first == it_e->first &&		//if the port number is correct
				(it_r->first).second == it_e->second) {	//and the seq number is the expected one
			it_e->second += 1;			//update the next expected sequence number
			Packet *pp = it_r->second;					//get the packet in the buffer
			struct hdr_cmn *ch = HDR_CMN(pp);
			ch->size() -= sizeof(hdr_uwtp_data);
			sendUp(id, pp);								//send the payload up
		}
	}
	int removed = 0;
	for (map<UWTPPair, Packet *>::iterator it_r = receiveBuffer.begin();
			it_r != receiveBuffer.end();
			) {				//iterate again over the receive buffer
		if ((it_r->first).first == it_e->first &&
				(it_r->first).second < it_e->second) {
			map<UWTPPair, Packet *>::iterator toErase = it_r;
			//receiveBuffer.erase(make_pair(it_e->first, it_e->second)); //remove the passed packets from the buffer
																		//but why not do it before
			++it_r;
			++removed;
			receiveBuffer.erase(toErase);
		}
		else ++it_r;
	}
	if(debug_) cout << "Packets removed from the receive queue: " << removed << endl;
}

void
UWTP::checkNack(int port_no, int seq_no)
{

	if (debug_) cout << "checkNack, seqNum = " << seq_no << endl;
		//cout << TIME << " UWTP::checkNack(" << node_id
		//	 << "), any packet which is received has nack packet saved in the "
		//		"nack buffer"
		//	 << endl;

	map<UWTPPair, NackPktStoreInfo *>::iterator it_nb =
			nackBuffer.find(make_pair(port_no, seq_no));		//find the desired nack in the queue

	if (it_nb != nackBuffer.end()) {
		nackBuffer.erase(make_pair(port_no, seq_no));			//if the nack is found, remove it from the buffer
	}

	if (debug_) { cout << "Content of the NACK buffer" << endl;
		//cout << TIME << " UWTP::checkNack(" << node_id
		//	 << "), Checking nack buffer " << endl;
		for (map<UWTPPair, NackPktStoreInfo *>::iterator it_nb =
						nackBuffer.begin();
				it_nb != nackBuffer.end();
				it_nb++) {
			cout << TIME << " Destination port : " << (it_nb->first).first
				 << " Seq no " << (it_nb->first).second << " nack tx info "
				 << it_nb->second->getNackTxInfo() << endl;
		}
	}

	//I dont think that checkNACK is a good name for this method, but whatever
}

void
UWTP::recvData(Packet *p, int id)
{

	if (debug_) cout << "Received a DATA packet. RECEIVER is " << node_id << endl;
		//cout << TIME << " UWTP::recvData(" << node_id
		//	 << "), a data packet is received" << endl;


	//read all the fields
	hdr_uwtp_data *uwtpdh = HDR_UWTP_DATA(p);
	hdr_cmn *cmh = HDR_CMN(p);
	int sport_no = uwtpdh->getSport();
	int dport_no = uwtpdh->getDport();
	int seq_no = cmh->uid();


	if (debug_) cout << "size of receive queue for " << dport_no << " is: " << receiveBuffer.size() << endl;

	if (debug_) {
		cout << "RICEVO: da " << dport_no - 1 << ", SN = " << seq_no << endl;
	//	cout << "Packet type " << uwtpdh->type_ << endl;
	//	cout << "SEQ NUM " << seq_no << endl;
	//	cout << "Source port " << sport_no << endl;
	//	cout << "Destination port " << dport_no << endl;
	}

	checkNack(dport_no, seq_no);	//check if there is a nack for this packet
									//if there is, remove it

	map<UWTPPair, UWTPPktStoreInfo *>::iterator it_dp;

	if (debug_) {
		cout << "expected seqNum for the INCOMING packets:" << endl;
		for (map<PortNo, ExpectedPktSeqNo>::iterator it_e = expPktInfo.begin();
				it_e != expPktInfo.end();
				it_e++) {
			cout << TIME << " Port no " << it_e->first << " expected seq no "
				 << it_e->second << endl;
		}
	}

	map<PortNo, ExpectedPktSeqNo>::iterator it_e = expPktInfo.find(dport_no);

	if (ack_mode == WITH_ACK && ack_tx_mode == WITHOUT_CUM_ACK) { //if there are per-packet ACKs
		Packet *ack_pkt = Packet::alloc();
		initAckPkt(p, ack_pkt, seq_no);
		sendAck(ack_pkt);						//create and send the ACK for the received packet
	}

	if (it_e == expPktInfo.end()) {			//if there is no entry in the map for dport_no
											//I guess this should happen only for the first received packet
		sendUp(id, p);
		expPktInfo.insert(make_pair(dport_no, seq_no + 1));		//create the entry with the next seq_no
		map<PortNo, ExpectedPktSeqNo>::iterator it_e =
				expPktInfo.find(dport_no);
	} else {								//if there already is an entry


		if (it_e->second > seq_no) {		//if the sequence number of the entry is bigger than the one of the packet
											//which means the received packet is too old
			if (debug_)
				cout << TIME
					 << "\n\n\nA packet is received with lower sequence number than "
						"expected one \n\n\n"
					 << endl;
			drop(p, 1, "LTESN"); // Less than expected sequence number = drop the packet
			return;

			
		} else if (it_e->second == seq_no) {	//if the sequence number is the expected one
			cmh->size() -= uwtpdh->size();
			Packet *rcvPkt = p->copy();
			sendUp(id, p);						//send the packet up
			//map<PortNo, ExpectedPktSeqNo>::iterator it_e =
			//		expPktInfo.find(dport_no);
			it_e->second += 1;			//update the next expected sequence number

			checkReceiveQueue(dport_no, id);	//check if in the buffer we have out of order packets
												//that can now be delivered in order

			if (ack_mode == WITH_ACK && ack_tx_mode == WITH_CUM_ACK) {	//if we are using cumulative ACKs

				//WEIRD THING OF PROTOCOL IS HERE
				//if (expected_ACK_threshold < RNV) {	//confront the value with a random value
				if (expPktInfo.find(dport_no)->second % cum_ack_parameter == 0) {
					Packet *ack_pkt = Packet::alloc();		//create the ACK packet
					map<PortNo, ExpectedPktSeqNo>::iterator it_e =
							expPktInfo.find(dport_no); 		//find the next sequence number
					hdr_uwtp_data *udh = HDR_UWTP_DATA(rcvPkt);
					initAckPkt(rcvPkt, ack_pkt, (it_e->second) - 1);
					sendAck(ack_pkt);						//send the cumulative ACK
				}
			}
			return;	//end of the if for the correct sequence number


		} else {	//if the received packet's sequence number is too big (out of order for the future)
			if(debug_) cout << "SEQUENCE NUMBER TOO BIG" << endl;
			if (nackBuffer.size() == 0) {	//if there is nothing in the nack buffer
				//delay_timer_.resched(JITTER * delay_interval);	//schedule the timer for the future
				delay_timer_.resched(delay_interval);
			}

			for (int i = it_e->second; i < seq_no; i++) {		//iterate from i = expected sequence number
																//to the received sequence number (excluded)
				map<UWTPPair, Packet *>::iterator it_rb =
						receiveBuffer.find(make_pair(dport_no, i));		//find if packet with seq_no equal to i 
																		//is already in the buffer
				map<UWTPPair, NackPktStoreInfo *>::iterator it_nb =
						nackBuffer.find(make_pair(dport_no, it_e->second));	//check if there is also the nack


				if (it_rb == receiveBuffer.end() && it_nb == nackBuffer.end()) {
					//if there is no such packet and no such nack

					Packet *nack_pkt = Packet::alloc();		//create the nack packet
					initNackPkt(p, nack_pkt, i);			
					nack_store_info = new NackPktStoreInfo;	//NEW = check for memory leaks??
					nack_store_info->setNackPnt(nack_pkt);
					nack_store_info->setNackTxInfo(FALSE);
					nack_store_info->setSenderId(id);
					nackBuffer.insert(
							make_pair(make_pair(dport_no, i), nack_store_info));
				}
			}//end of for

			map<UWTPPair, Packet *>::iterator it_r =
					receiveBuffer.find(make_pair(dport_no, seq_no)); //check if we already received this packet

			if (it_r == receiveBuffer.end()) {		//if we received it for the first time

				if (receiveBuffer.size() >= receive_buffer_size) { //if the buffer is full
					int lowest_seq_no = MAX_PORT_NO;				//why use MAX_PORT_NO bah
					for (map<UWTPPair, Packet *>::iterator it_rc =
									receiveBuffer.begin();
							it_r != receiveBuffer.end();
							it_r++) {						//iterate over the receive buffer
						if ((it_rc->first).first == dport_no &&
								(it_rc->first).second < lowest_seq_no) {
							lowest_seq_no = (it_rc->first).second;		//look for the lowest sequence number
																		//for a packet for this port
						}
					}

					map<UWTPPair, Packet *>::iterator it_r1 =
							receiveBuffer.find(
									make_pair(dport_no, lowest_seq_no));	//pick the packet with the found
																			//lowest sequence number
					if (it_r1 != receiveBuffer.end()) {						//if it exists, then
						map<PortNo, ExpectedPktSeqNo>::iterator it_e =
								expPktInfo.find(dport_no);			//find the expected sequence number for this port
						it_e->second = lowest_seq_no;				//update it with the lowest we have available in the
																	//receive buffer
						checkReceiveQueue(dport_no, id);			//try to empty the receive buffer since it's full
					}
					cout << "\n\n\nADDING TO THE BUFFER" << endl;
					receiveBuffer.insert(
							make_pair(make_pair(dport_no, seq_no), p));//put the new packet in the buffer


				} else { //if the buffer is not full
					cout << "\n\n\nADDING TO THE BUFFER" << endl;

					receiveBuffer.insert(						//just insert the new packet
							make_pair(make_pair(dport_no, seq_no), p));
				}	//code here can be formatted better

			} //end of "if received for the first time"
		}//end of "if the packet seq_no is too big"
	}//end of "if we already know the expected sequence number"


	if (sendBuffer.size() > 0) {	//if we have something to send
		for (map<UWTPPair, UWTPPktStoreInfo *>::iterator it_p =
						sendBuffer.begin();
				it_p != sendBuffer.end();
				it_p++) {		//iterate over the send buffer
			if (it_p->second->getTimeSpentInQueue() >
					pkt_delete_time_from_queue) {	//in case of ack packet loss, remove very old packets
				sendBuffer.erase(it_p->first);
			}
		}
	}
}


void
UWTP::recvAck(Packet *p)
{

	if (debug_) cout << "ACK received, seqNum = ";
		//cout << TIME << " UWTP::rcvAck(" << node_id
		//	 << "), an ACK packet is received" << endl;

	hdr_uwtp_ack *uwtpah = HDR_UWTP_ACK(p);
	if(debug_) cout << uwtpah->getSeqNo() << endl;
	ack_rx_count++;

	if (ack_tx_mode == WITHOUT_CUM_ACK) {	//if using per-packet ACKs

		map<UWTPPair, UWTPPktStoreInfo *>::iterator it_p = sendBuffer.find(
				make_pair(uwtpah->getSport(), uwtpah->getSeqNo()));	//look for the packet relative to the ACK

		
		if (it_p != sendBuffer.end()) {		//if it exists
			sendBuffer.erase(it_p->first);	//remove it from the buffer
		} else {
			cout << TIME << " A wrong packet is received" << endl;
		}


	} else if (ack_tx_mode == WITH_CUM_ACK) {	//if using cumulative ACKs
		int count = 0;
		int p = 0;
		for (map<UWTPPair, UWTPPktStoreInfo *>::iterator it_p =
						sendBuffer.begin();
				it_p != sendBuffer.end();
				/*nothing here*/) {		//iterate over the send buffer
			//cout << count << endl;
			if ((uint)(it_p->first).first == (uint) uwtpah->getSport() &&
					(uint) (it_p->first).second <= (uint) uwtpah->getSeqNo()) {	//if the packet is in the ACKed range
				count++;
				//it_p = sendBuffer.erase(it_p->first);		//remove it from the buffer 
													//QUESTA RIGA CAUSAVA UN SEGMENTATION FAULT

				map<UWTPPair, UWTPPktStoreInfo*>::iterator toErase = it_p;
        		++it_p;		//need to do this before ereasing because the erase() invalidates the iterator
        		sendBuffer.erase(toErase);
			}
			else{++it_p;}

			if(debug_) cout << "Packets acked: " << count << endl;
		}
		if (debug_)
			cout << TIME << " No of pkt deleted from buffer " << count << endl;
	
	
		} else {
		cout << TIME << " Wrong ack tx mode is selected " << endl;
	}
}

void
UWTP::recvNack(Packet *p)
{

	if (debug_) cout << "NACK received, seqNum = ";
		//cout << TIME << " UWTP::recvNack(" << node_id
		//	 << "), a Nack packet is received" << endl;

	nack_rx_count++;

	hdr_uwtp_nack *uwtpnah = HDR_UWTP_NACK(p);
	if(debug_) cout << uwtpnah->getSeqNo() << endl;

	if(debug_) cout << uwtpnah->getSport() << ", " << uwtpnah->getSeqNo() << endl;
	if(debug_) cout << "sendbuffer size is " << sendBuffer.size() << endl;
	for (map<UWTPPair, UWTPPktStoreInfo *>::iterator it_p =
									sendBuffer.begin();
							it_p != sendBuffer.end();
							it_p++){
								cout << it_p->first.second << endl;
							}

	map<UWTPPair, UWTPPktStoreInfo *>::iterator it_p = sendBuffer.find(
			make_pair(uwtpnah->getSport(), uwtpnah->getSeqNo()));		//find the NACKed packet in the buffer

	if (it_p == sendBuffer.end()) { //2222
		cout << "The packet cant be retxed because it's not in the buffer anymore. Abort" << endl;
		return;
	}
	Packet *curr_data_pkt = (it_p->second->getPktPnt())->copy();


	if (it_p != sendBuffer.end()) {		//if it exists, retransmit it
		if(debug_) cout << "----- retransmittinig the packet -----" << endl;
		sendDown(curr_data_pkt);
	} else {
		cout << TIME << " A NACK was received for a packet not in the buffer" << endl;
	}



	//3333
	//versione migliorata con un pezzo di codice copiato da recvACK
	//il nack funge anche da ACK cumulativo, indipendentemente dalla modalita' di trasmissione
	//in quanto il numero del nack indica che tutti i pacchetti precedenti a quello sono ricevuti correttamente
	//l'unica differenza e' che il minore uguale e' sostituito con un minore stretto
	int count = 0;
	for (map<UWTPPair, UWTPPktStoreInfo *>::iterator it_p =
					sendBuffer.begin();
			it_p != sendBuffer.end();
			/*nothing here*/) {		//iterate over the send buffer
		if ((uint)(it_p->first).first == (uint) uwtpnah->getSport() &&
				(uint) (it_p->first).second < (uint) uwtpnah->getSeqNo()) {	//if the packet is in the ACKed range
			count++;
			map<UWTPPair, UWTPPktStoreInfo*>::iterator toErase = it_p;
			++it_p;		//need to do this before ereasing because the erase() invalidates the iterator
			sendBuffer.erase(toErase);
		}
		else{++it_p;}
	}

		if(debug_) cout << "Packets acked: " << count << endl;
}

void
UWTP::recv(Packet *p, int idSrc)
{

	hdr_cmn *ch = HDR_CMN(p);

	if (debug_){
		cout << "\n\n\n==================================================\nPARLA " << getId() << endl;
		if(ch->direction() == hdr_cmn::UP) cout << "packet UP" << endl;
		else cout << "packet DOWN" << endl;
		//cout << TIME << " UWTP::recv(" << node_id
		//	 << "), a data/ack/nack packet is received from upper layer or "
		//		"from lower layer"
		//	 << endl;
	}
	

	

	if (!ch->error()) {		//if the packet is correct

		if (ch->direction() == hdr_cmn::UP) { //if the packet is being received

			struct hdr_uwtp *uwtph = HDR_UWTP(p);
			struct hdr_uwtp_data *uwtpdh = HDR_UWTP_DATA(p);

			map<int, int>::const_iterator iter =
					id_map.find(uwtpdh->getDport());	//find the port number

			if (iter == id_map.end()) {
				// Unknown Port Number
				cout << TIME
					 << "UWtrans::recv() (dir:UP), receive a packet with "
						"unrecognized port number"
					 << endl;
				drop(p, 1, "UPN");
				return;
			}

			int id = iter->second;
			//if (debug_)
			//	cout << "dest port " << uwtpdh->getDport() << "id of src " << id
			//		 << endl;

			switch (uwtph->type_) {

				case (UWTP_DATA):
					if(debug_) cout << "PACCHETTO DI DATI RICEVUTO" << endl;
					recvData(p, id);
					break;

				case (UWTP_ACK):
					if(debug_) cout << "ACK RICEVUTO" << endl;
					recvAck(p);
					break;

				case (UWTP_NACK):
					if(debug_) cout << "\n\nNACK RICEVUTO\n\n" << endl;
					recvNack(p);
					break;
			}

		} else { //if the packet is being sent

			struct hdr_uwtp_data *uwtpdh = HDR_UWTP_DATA(p);

			map<UWTPPair, UWTPPktStoreInfo *>::iterator it_p =
					sendBuffer.find(make_pair(uwtpdh->getDport(), ch->uid())); //check if that seq num is already in the buffer

			if (it_p != sendBuffer.end()) {	//if already in the buffer
				if (debug_)
					cout << TIME
						 << " This packet is already stored in the buffer "
						 << endl;
			} else {		//if not in the buffer
				// its a new packet for the destination
				++seq_no_counter;
				initPkt(p, idSrc, seq_no_counter);
				if(debug_) cout << "A PACKET IS BEING SENT; SOURCE is " << idSrc << "\nSENDBUFFER SIZE = " << sendBuffer.size() << endl;
				if(debug_) cout << "spazio totale " << send_buffer_size << endl;

				if (sendBuffer.size() < send_buffer_size) {	//if we still have space in the buffer
					if(debug_) cout << "C'E SPAZIO LIBERO" << endl;
					pkt_store_info = new UWTPPktStoreInfo;
					pkt_store_info->setPktStoreTime(TIME);
					pkt_store_info->setPktPnt(p);
					pkt_store_info->setPktTxInfo(FALSE);

					sendBuffer.insert(		//just add the packet to the buffer
							make_pair(make_pair(uwtpdh->getDport(), ch->uid()),
									pkt_store_info));
					if (debug_) {
						cout << "Content of sendBuffer:" << endl;
						for (map<UWTPPair, UWTPPktStoreInfo *>::iterator it =
										sendBuffer.begin();
								it != sendBuffer.end();
								it++) {
							cout << TIME << " port no " << (it->first).first
								 << "; seq no " << (it->first).second
								 << "; pkt store time "
								 << (it->second)->getPktStoreTime() << endl;
								 //<< "; pkt pointer "
								 //<< (it->second)->getPktPnt() << endl;
						}
					}

				} else {	//if the buffer is full
					cout << "\nIL BUFFER E PIENO" << endl;
					double highest_waiting_time = 0;	//find the oldest packet in the buffer
					int port_, sno_;
					map<UWTPPair, UWTPPktStoreInfo *>::iterator toErase = sendBuffer.begin();
					for (map<UWTPPair, UWTPPktStoreInfo *>::iterator it_p =
									sendBuffer.begin();
							it_p != sendBuffer.end();
							it_p++) {
						if (it_p->second->getTimeSpentInQueue() >
								highest_waiting_time) {
							highest_waiting_time =
									it_p->second->getTimeSpentInQueue();
							//port_ = uwtpdh->getDport(); 		//wtf why would you save the port and the 1111
							//sno_ = ch->uid();					//id of the packet to send instead of the old one
							port_ = it_p->first.first;
							sno_ = it_p->first.second;
							toErase = it_p;
							cout << toErase->first.first << ", " << toErase->first.second << endl;
						}
					}
					assert(port_ >= 0 && sno_ >= 0);
					cout << "we are removing port: " << port_ << ", sno: " << sno_ << endl;
					cout << toErase->first.first << ", " << toErase->first.second << endl;
					cout << "dimensione del buffer: " << sendBuffer.size() << endl;
					//sendBuffer.erase(make_pair(port_, sno_));	//remove the oldest packet to make some space
					sendBuffer.erase(toErase);
					cout << "dimensione del buffer dopo la rimozione: " << sendBuffer.size() << endl;
					pkt_store_info = new UWTPPktStoreInfo;
					pkt_store_info->setPktStoreTime(TIME);
					pkt_store_info->setPktPnt(p);
					pkt_store_info->setPktTxInfo(FALSE);
					sendBuffer.insert(							//add the new packet
							make_pair(make_pair(uwtpdh->getDport(), ch->uid()),
									pkt_store_info));
				}

				for (map<UWTPPair, UWTPPktStoreInfo *>::iterator it_p =
								sendBuffer.begin();
						it_p != sendBuffer.end();
						it_p++) {	//iterate over the send buffer
					if (it_p->second->getPktTxInfo() == FALSE) {	//if there is any packet in the send buffer
																	//that has not been sent yet
						it_p->second->setPktTxInfo(TRUE);			//send it
						Packet *curr_data_pkt =
								(it_p->second->getPktPnt())->copy();
						sendDown(curr_data_pkt);
					}
				}
			}
		}
	}
}

int
UWTP::assignPort(Module *m)
{
	int id = m->getId();

	// Check that the provided module has not been given a port before
	if (port_map.find(id) != port_map.end())
		return TCL_ERROR;

	int newport = ++portcounter;

	port_map[id] = newport;
	assert(id_map.find(newport) == id_map.end());
	id_map[newport] = id;
	assert(id_map.find(newport) != id_map.end());

	if (debug_)
		std::cout << "PortMap::assignPort() "
				  << " id=" << id << " port=" << newport
				  << " portcounter=" << portcounter << std::endl;
	return newport;
}

int
UWTP::getUWTPDataHSize()
{
	return sizeof(hdr_uwtp_data);
}

int
UWTP::getUWTPAckHSize()
{
	return sizeof(hdr_uwtp_ack);
}

int
UWTP::getUWTPNackHSize()
{
	return sizeof(hdr_uwtp_nack);
}
