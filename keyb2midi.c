#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/select.h>
//#include <vector>
#include <alsa/asoundlib.h>


int MIDI_BUFFER_SIZE = 4; // only note on and off
int VERBOSE = 0;

snd_seq_t * alsa_seq = 0;
int alsa_port = 0;

static int send_midi_on(int code);
static int send_midi_off(int code);
static int setup_alsa(char * keyb_name); 

static void usage()
{
  printf("options: -k <keyboard name> - use ls /dev/input/by-id to find the keyboard names \n");
  printf("         -h - display this text\n");
  printf("         -x - display example xorg.xconf to prevent X input (use lsusb to find the ID)\n");
  printf("         -v - verbose, print key on and off events\n");
  exit(1);
}
static void dump_xorg_conf()
{
  printf("Section \"InputClass\"\n");
  printf("    Identifier      \"disablekeybd\"\n");
  printf("    MatchUSBID      \"1111:0000\"\n");
  printf("    Option          \"Ignore\" \"true\"\n");
  printf("EndSection\n");
  exit(0);  
}
int main(int argc, char **argv)
{

  char * keyb_name = "usb-USB_USB_Keykoard-event-kbd";
  
  // parse command line
  int c;
  while ( ( c = getopt(argc, argv, "k:hxv") ) != EOF) {
    switch (c) {
      default:
      case 'h': usage(); break;
      case 'x': dump_xorg_conf(); break;
      case 'k': keyb_name = optarg; break;
      case 'v': VERBOSE = 1; break;
    }
  }
    
  printf("keyb2midi by teknopaul\n");

  char device[1024];
  memset(device, 0, 1024);
  strcat(device, "/dev/input/by-id/");
  strncat(device, keyb_name, 1000);

  struct stat stat_s;
  
  int stat_ret = stat(device, &stat_s);
  
  if (stat_ret != 0) {
    fprintf(stderr, "Unable to stat file %s\n", device);
    return 1;
  }
 
    // open file to read
  int in = open(device, O_RDONLY);
  if (in == -1) {
    fprintf(stderr, "Unable to open %s\n", device);
    return 3;
  }
  
  int ret = setup_alsa(keyb_name);
  if (ret != 0) {
    return ret;
  }
    
  struct input_event ev[64];
  
  int size = sizeof(struct input_event);
  int red = 0;
  while (1){
    if ((red = read(in, ev, size * 64)) < size)
      printf("err\n");     
 
      int value = ev[0].value;
      
      //printf ("type %x\n", (ev[1].type));
      //printf ("code %x\n", (ev[1].code));
      //printf ("value %x\n", (ev[1].value));
 
      if (value != ' ' && ev[1].value == 1 && ev[1].type == 1) { // key on
	if (VERBOSE) printf ("Note on %x\n", ev[1].code );
	send_midi_on(ev[1].code);
      }

      if (value != ' ' && ev[1].value == 0 && ev[1].type == 1) { // key off
	if (VERBOSE) printf ("Note off %x\n", ev[1].code );
	send_midi_off(ev[1].code);
      }

  }
 
  return 0;
}


static int send_midi_on(int code)
{
  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, alsa_port);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_direct(&ev);
  snd_seq_ev_set_noteon(&ev, 0, code, 100); // velocity hardcoded to 100

  snd_seq_event_output_direct(alsa_seq, &ev);
  return 0;
}

static int send_midi_off(int code)
{
  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, alsa_port);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_direct(&ev);
  snd_seq_ev_set_noteoff(&ev, 0, code, 0);
  snd_seq_event_output_direct(alsa_seq, &ev);  
  return 0;
}

static int setup_alsa(char * keyb_name) 
{
  
  // Setup Alsa

  int alsa_err;
  if ( ( alsa_err = snd_seq_open(&alsa_seq, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK)) <  0) {
      fprintf(stderr, "Could not init ALSA sequencer: %s\n", snd_strerror(alsa_err));
      alsa_seq = 0;
      return 1;
  } else {
      printf ("Initialised ALSA sequencer: %s\n", keyb_name);
  }
  
  snd_seq_set_client_name(alsa_seq, keyb_name);

  char buf[16];

  alsa_port = snd_seq_create_simple_port(alsa_seq, buf,
		  SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
		  SND_SEQ_PORT_TYPE_APPLICATION);
    
  if (alsa_port < 0) {
    fprintf(stderr, "Could not create ALSA sequencer port\n");
    return 1;
  }

  return 0;
}

