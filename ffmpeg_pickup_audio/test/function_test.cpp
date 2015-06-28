#include "../src/pickup_audio.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("usage: pickup_audio input_file output_file\n");
		return -1;
	}
	av_register_all();
	avformat_network_init();
	CPickupAudio pickup;
	pickup.Start(argv[1], argv[2]);
	while (pickup.IsRunning())
	{
		usleep(1000);
	}
	pickup.Stop();
	return 0;
}