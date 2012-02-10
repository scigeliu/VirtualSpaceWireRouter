#include "VirtualSpaceWireRouter.hh"

int main(int argc, char* argv[]) {
	using namespace std;
	VirtualSpaceWireRouter* router = new VirtualSpaceWireRouter("192.168.1.100", 10030);
	router->start();
	CxxUtilities::Condition c;
	c.wait();
}
