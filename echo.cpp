#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

#include <sodium/utils.h>
#include <tox/tox.h>
#include <tox/toxav.h>
#include <thread>    


Tox*tox;
typedef struct DHT_node {
    const char *ip;
    uint16_t port;
    const char key_hex[TOX_PUBLIC_KEY_SIZE*2 + 1];
    unsigned char key_bin[TOX_PUBLIC_KEY_SIZE];
} DHT_node;

const char *savedata_filename = "savedata.tox";
const char *savedata_tmp_filename = "savedata.tox.tmp";

Tox *create_tox()
{
    Tox *tox;

    struct Tox_Options options;

    tox_options_default(&options);

    FILE *f = fopen(savedata_filename, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        char *savedata = (char*) malloc(fsize);

        fread(savedata, fsize, 1, f);
        fclose(f);

        options.savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
        options.savedata_data = (const uint8_t*) savedata;
        options.savedata_length = fsize;

        tox = tox_new(&options, NULL);

        free(savedata);
    } else {
        tox = tox_new(&options, NULL);
    }

    return tox;
}

void update_savedata_file(const Tox *tox)
{
    size_t size = tox_get_savedata_size(tox);
    char *savedata = (char*) malloc(size);
    tox_get_savedata(tox, (uint8_t*) savedata);

    FILE *f = fopen(savedata_tmp_filename, "wb");
    fwrite(savedata, size, 1, f);
    fclose(f);

    rename(savedata_tmp_filename, savedata_filename);

    free(savedata);
}

void bootstrap(Tox *tox)
{
    DHT_node nodes[] =
    {
        {"178.62.250.138",             33445, "788236D34978D1D5BD822F0A5BEBD2C53C64CC31CD3149350EE27D4D9A2F9B6B", {0}},
        {"2a03:b0c0:2:d0::16:1",       33445, "788236D34978D1D5BD822F0A5BEBD2C53C64CC31CD3149350EE27D4D9A2F9B6B", {0}},
        {"tox.zodiaclabs.org",         33445, "A09162D68618E742FFBCA1C2C70385E6679604B2D80EA6E84AD0996A1AC8A074", {0}},
        {"163.172.136.118",            33445, "2C289F9F37C20D09DA83565588BF496FAB3764853FA38141817A72E3F18ACA0B", {0}},
        {"2001:bc8:4400:2100::1c:50f", 33445, "2C289F9F37C20D09DA83565588BF496FAB3764853FA38141817A72E3F18ACA0B", {0}},
        {"128.199.199.197",            33445, "B05C8869DBB4EDDD308F43C1A974A20A725A36EACCA123862FDE9945BF9D3E09", {0}},
        {"2400:6180:0:d0::17a:a001",   33445, "B05C8869DBB4EDDD308F43C1A974A20A725A36EACCA123862FDE9945BF9D3E09", {0}},
        {"biribiri.org",               33445, "F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D822E8460FAB67", {0}}
    };

    for (size_t i = 0; i < sizeof(nodes)/sizeof(DHT_node); i ++) {
        sodium_hex2bin(nodes[i].key_bin, sizeof(nodes[i].key_bin),
                       nodes[i].key_hex, sizeof(nodes[i].key_hex)-1, NULL, NULL, NULL);
        tox_bootstrap(tox, nodes[i].ip, nodes[i].port, nodes[i].key_bin, NULL);
    }
}

void print_tox_id(Tox *tox)
{
    uint8_t tox_id_bin[TOX_ADDRESS_SIZE];
    tox_self_get_address(tox, tox_id_bin);

    char tox_id_hex[TOX_ADDRESS_SIZE*2 + 1];
    sodium_bin2hex(tox_id_hex, sizeof(tox_id_hex), tox_id_bin, sizeof(tox_id_bin));

    for (size_t i = 0; i < sizeof(tox_id_hex)-1; i ++) {
        tox_id_hex[i] = toupper(tox_id_hex[i]);
    }

    printf("Tox ID: %s\n", tox_id_hex);
}

void friend_request_cb(Tox *tox, const uint8_t *public_key, const uint8_t *message, size_t length,
                       void *user_data)
{
    tox_friend_add_norequest(tox, public_key, NULL);

    update_savedata_file(tox);
}

void friend_message_cb(Tox *tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t *message,
                       size_t length, void *user_data)
{
    tox_friend_send_message(tox, friend_number, type, message, length, NULL);
}

void self_connection_status_cb(Tox *tox, TOX_CONNECTION connection_status, void *user_data)
{
    switch (connection_status) {
    case TOX_CONNECTION_NONE:
        printf("Offline\n");
        break;
    case TOX_CONNECTION_TCP:
        printf("Online, using TCP\n");
        break;
    case TOX_CONNECTION_UDP:
        printf("Online, using UDP\n");
        break;
    }
}

void dummy_thread( uint32_t friend_number){

	size_t dummy_length = 1000;
	float sleeptime = 0.01;
	uint8_t *dummy_data = (uint8_t*) malloc(dummy_length);
	tox_friend_send_lossy_packet(tox, friend_number, (const uint8_t *)dummy_data, dummy_length,NULL);
}

void toxav_call_callback(ToxAV *av, uint32_t friend_number, bool audio_enabled, bool video_enabled, void *user_data){

	 printf("Incoming Call\n");
	 toxav_answer(av, friend_number,64, 0,NULL);
	 printf("Accepting Incoming Call\n");
    	 std::thread dummy_handle (dummy_thread, friend_number);
}


//Just Echo Audio Back
void toxav_on_audio_receive_frame(ToxAV *av, uint32_t friend_number, const int16_t *pcm, size_t sample_count,
		uint8_t channels, uint32_t sampling_rate, void *user_data){

	toxav_audio_send_frame(av,friend_number, pcm, sample_count, channels,sampling_rate,NULL);

}


void av_thread(ToxAV* tox_av){


	while(1){
		toxav_iterate(tox_av);
        	usleep(toxav_iteration_interval(tox_av) * 1000);
	}
}


int main()
{
    tox = create_tox();
    ToxAV *tox_av = toxav_new(tox,NULL);


    std::string name = "Echo Bot";
    tox_self_set_name(tox, (const uint8_t*) name.c_str(), strlen(name.c_str()), NULL);

    std::string status_message = "Echoing your messages";
    tox_self_set_status_message(tox, (const uint8_t*) status_message.c_str(), strlen(status_message.c_str()), NULL);

    bootstrap(tox);

    print_tox_id(tox);

    tox_callback_friend_request(tox, friend_request_cb);
    tox_callback_friend_message(tox, friend_message_cb);

    tox_callback_self_connection_status(tox, self_connection_status_cb);

    toxav_callback_call(tox_av, toxav_call_callback, NULL);
    toxav_callback_audio_receive_frame(tox_av, toxav_on_audio_receive_frame,NULL);

    update_savedata_file(tox);

    std::thread av_thread_handle (av_thread,tox_av);

    while (1) {
        tox_iterate(tox, NULL);
        usleep(tox_iteration_interval(tox) * 1000);
    }

    toxav_kill(tox_av);
    tox_kill(tox);

    av_thread_handle.join();

    return 0;
}


