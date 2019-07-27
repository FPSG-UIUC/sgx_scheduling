
#include"util.hpp"

int main(int argc, char **argv)
{
	// Put your covert channel setup code here

	printf("Please type a message.\n");

	bool sending = true;
	while (sending) {
		char text_buf[128];
		fgets(text_buf, sizeof(text_buf), stdin);
	
		// Put your covert channel code here
	}

	printf("Sender finished.\n");

	return 0;
}


