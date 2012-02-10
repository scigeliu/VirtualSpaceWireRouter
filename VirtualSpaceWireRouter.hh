/*
 * VirtualSpaceWireRouter.hh
 *
 *  Created on: Feb 10, 2012
 *      Author: yuasa
 */

#include "CxxUtilities/Thread.hh"
#include "CxxUtilities/Time.hh"
#include "SpaceWire.hh"

class Port: public CxxUtilities::StoppableThread {
public:
	enum {
		ServerPort, ClientPort
	};
protected:
	SpaceWireIF* spwif;
public:
	std::vector<uint8_t> *receivedPacket;
	CxxUtilities::Condition* transferredCondition;
	CxxUtilities::Condition* receiveCondition;
	uint32_t mode;
	std::string url;
	std::string portName;
	uint32_t port;
public:
	bool spwifOpened;

public:
	Port(std::string portName, CxxUtilities::Condition& receiveCondition,
			CxxUtilities::Condition& transferredCondition, uint32_t port) {//server mode
		spwifOpened = false;
		this->spwif = spwif;
		this->mode = ServerPort;
		this->portName = portName;
		this->port = port;
		this->receiveCondition = &receiveCondition;
		this->transferredCondition = &transferredCondition;
	}
	Port(std::string portName, CxxUtilities::Condition& receiveCondition,
			CxxUtilities::Condition& transferredCondition, std::string url, uint32_t port) {//client mode
		spwifOpened = false;
		this->spwif = spwif;
		this->mode = ClientPort;
		this->url = url;
		this->portName = portName;
		this->port = port;
		this->receiveCondition = &receiveCondition;
		this->transferredCondition = &transferredCondition;
	}
public:
	void run() {
		using namespace std;
		if (mode == ServerPort) {
			cout << "Waiting a connection on " << portName << "..." << endl;
			spwif = new SpaceWireIFOverTCP(port);
		} else {
			cout << "Connecting " << portName << "..." << endl;
			spwif = new SpaceWireIFOverTCP(url, port);
		}
		loop_open: try {
			spwif->open();
		} catch (...) {
			cerr << portName << " timeout. Retrying..." << endl;
			goto loop_open;
		}
		spwifOpened = true;
		cout << portName << " has been connected." << endl;

		stopped = false;
		while (!stopped) {
			receivedPacket = spwif->receive();
			receiveCondition->signal();
			transferredCondition->wait();
		}
		spwifOpened = false;
	}
private:
	CxxUtilities::Mutex sendMutex;
public:
	void send(std::vector<uint8_t>* data) {
		sendMutex.lock();
		spwif->sendVectorPointer(data);
		sendMutex.unlock();
	}
public:
	void close() {
		spwif->close();
	}
};

class VirtualSpaceWireRouter: public CxxUtilities::StoppableThread {
public:
	class ReceiveThread: public CxxUtilities::StoppableThread {
	private:
		VirtualSpaceWireRouter* parent;
		uint8_t watchingPort;
	public:
		ReceiveThread(VirtualSpaceWireRouter* parent, uint8_t watchingPort) {
			this->parent = parent;
			this->watchingPort = watchingPort;
		}
	public:
		void run() {
			using namespace std;
			stopped = false;
			while (!stopped) {
				parent->receiveConditions[watchingPort].wait();//wait for a packet
				//received
				parent->nReceivedPackets[watchingPort]++;
				parent->routePacket(parent->ports[watchingPort]->receivedPacket,watchingPort);
			}
		}
	};

protected:
	std::vector<Port*> ports;
	std::vector<ReceiveThread*> receveThreads;
	std::vector<CxxUtilities::Condition> receiveConditions;
	std::vector<CxxUtilities::Condition> transferredConditions;
	std::vector<uint32_t> nSentPackets;
	std::vector<uint32_t> nReceivedPackets;

public:
	VirtualSpaceWireRouter(std::string url, uint32_t port) {
		using namespace std;
		cout << "--------------------------------" << endl;
		cout << "VirtualSpaceWireRouter on Mac/PC" << endl;
		cout << "--------------------------------" << endl;

		ports.resize(MaxPortNumber + 1);
		nSentPackets.resize(MaxPortNumber + 1);
		nReceivedPackets.resize(MaxPortNumber + 1);
		receiveConditions.resize(MaxPortNumber + 1);
		transferredConditions.resize(MaxPortNumber + 1);
		receveThreads.resize(MaxPortNumber + 1);

		ports[0] = NULL;
		ports[1] = new Port("Port1 (to SpaceWire-to-GigabitEther)", receiveConditions[1], transferredConditions[1],
				url, 10030);//client
		ports[2] = new Port("Port2", receiveConditions[2], transferredConditions[2], 10031);//server
		ports[3] = new Port("Port3", receiveConditions[3], transferredConditions[3], 10032);//server
		ports[4] = new Port("Port4", receiveConditions[4], transferredConditions[4], 10033);//server
		ports[5] = new Port("Port5", receiveConditions[5], transferredConditions[5], 10034);//server

		for (size_t i = 1; i <= MaxPortNumber; i++) {
			ports[i]->start();
			receveThreads[i] = new ReceiveThread(this, i);
			receveThreads[i]->start();
		}
	}
public:
	const static uint8_t MaxPortNumber = 5;
	const static uint8_t PortNumberOfClientPort = 1;

public:
	void run() {
		stopped = false;
		while (!stopped) {
			using namespace std;
			sleep(1000);
			cout << endl;
			cout << CxxUtilities::Time::getCurrentTimeAsString() << endl;
			cout << "Port | Incoming packets | Outgoing packets" << endl;
			for (size_t i = 1; i <= MaxPortNumber; i++) {
				cout << " " << i << "   " << nReceivedPackets[i] << "    " << nSentPackets[i] << endl;
			}
		}
	}

private:
	CxxUtilities::Mutex routingMutex;

public:
	void routePacket(std::vector<uint8_t>* data, uint8_t fromPort) {
		routingMutex.lock();

		using namespace std;
		uint8_t pathAddress;
		vector<uint8_t>::iterator it=data->begin();

		//check size
		if (data->size() == 0 || data->size() == 1) {
			goto finalize_routePacket;
		}

		//judge path address
		pathAddress = data->at(0);
		if (MaxPortNumber < pathAddress) {
			goto finalize_routePacket;
		}

		//if configuration port access, just discard
		if (pathAddress == 0) {
			goto finalize_routePacket;
		}

		//delete one byte
		data->erase(it);

		try {
			if (ports[pathAddress]->spwifOpened == true) {
				//send the packet if the destination port is operational
				ports[pathAddress]->send(data);
				cout << "### from " << (uint32_t)fromPort << " to " << (uint32_t)pathAddress << endl;
				SpaceWireUtilities::dumpPacket(data);
				nSentPackets[pathAddress]++;
			} else {
				cerr << "Packet to " << ports[pathAddress]->portName
						<< " was discarded because the port is not operational." << endl;
				goto finalize_routePacket;
			}
		} catch (SpaceWireIFException e) {
			if (e.getStatus() == SpaceWireIFException::Disconnected) {
				cerr << "SpaceWire-to-GigabitEther was disconnected." << endl;
				finalize();
			} else if (e.getStatus() == SpaceWireIFException::Timeout) {
				cerr << "Timeout happened in the SpaceWire-to-GigabitEther connection." << endl;
				goto finalize_routePacket;
			}
		}
		finalize_routePacket: transferredConditions[fromPort].signal();
		delete data;
		routingMutex.unlock();
	}

	void finalize() {
		using namespace std;
		cout << "Closing all ports." << endl;
		for (size_t i = 1; i < ports.size(); i++) {
			ports[i]->close();
			delete ports[i];
		}
	}
};
