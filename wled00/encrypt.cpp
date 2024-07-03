#include <AES.h>
#include <sha256.h>
#include "base64.h"
#include "wled.h"
// #include "printf.h"

// The AES library object.
AES aes;

// Our AES key. Note that is the same that is used on the Node-Js side but as hex bytes.
// byte aesKey[] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };
// byte aesKey[16] = "";
void encryptSetup() {
    String aesString = getSetting("aesKey");
    char_array_to_byte_array(aesString.c_str(), aesKey, aesString.length());
}

void setupEncryption() {
}
// The unitialized Initialization vector

// Our message to encrypt. Static for this example.
// String msg = "{\"data\":{\"value\":300}, \"SEQN\":700 , \"msg\":\"IT WORKS!!\" }";
// TODO- IMP - What if the aesString is less then 32 in size - this is a chaos. Fix it up.
// Its possible in cases of absesnce of aesKey

// Function for converting hex string into byte array
void char_array_to_byte_array(const char *str, byte byte_arr[], uint8_t string_length) {
    int index, index1, index2;
    char temp[3];
    // Sometimes we are sending 32 as fixed in string_length, but if str does not support that than we should return from here. 
    if(strlen(str) < string_length){
        return ; 
    }
    //   Serial.println(str);
    // Use 'nullptr' or 'NULL' for the second parameter.
    for (index = 0; index < string_length - 1; index += 2) {
        for (index1 = index, index2 = 0; index1 < index + 2; index1++, index2++) {
            temp[index2] = str[index1];
        }
        temp[index2] = '\0';
        unsigned long number = strtoul(temp, nullptr, 16);

        byte_arr[index / 2] = (byte)number;
    }
}
void temp_char_string_to_byte(const char *str, uint8_t byte_arr[], uint8_t string_length) {
    for (int i = 0; i < string_length; i++) {
        byte_arr[i] = (uint8_t)str[i];
    }
}

void encryptMsg(const char *msg, char *data_encoded, char *iv_encoded, byte *aesKey) {
    // char b64data[500];

    byte cipher[getCipherlength(strlen(msg)) + 1];
    // byte cipher[500];
    memset(cipher, 0, sizeof(cipher));

    byte my_iv[N_BLOCK] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int message_len = strlen(msg);
    aes.set_key(aesKey, sizeof(aesKey));  // Get the globally defined key
    gen_iv(my_iv);                        // Generate a random IV
    base64_encode(iv_encoded, (char *)my_iv, N_BLOCK);
    aes.do_aes_encrypt((byte *)msg, message_len, cipher, aesKey, 128, my_iv);

    base64_encode(data_encoded, (char *)cipher, aes.get_size());
}

void encryptMsg(const char *msg, char *data_encoded, char *iv_encoded) {
    return encryptMsg(msg, data_encoded, iv_encoded, aesKey);
}

// Arihant - Change this function as well - to use base64 encoding only once instead of twice. Change the same on server.
void decryptMsg(const char *payload, char *message, int payload_len, const char *iv_encoded, byte *aesKey) {
    // DEBUG_MSG_P(PSTR("Lets Decryot the message \n"));
    // DEBUG_MSG_P(PSTR("Encoded IV : %s \n"),iv_encoded );
    // DEBUG_MSG_P(PSTR("Encoded Message : %s \n"),payload );
    // Serial.println("Encoded Message : "+ String(payload));
    // Serial.println("Encoded IV : "+ String(iv_encoded));

    // Serial.println("Starting decryption");
    // Serial.println(millis());
    // unsigned long micros_begin = micros();
    byte iv_decoded[16];
    byte cipher_decrypted[payload_len + 1];  // Payload Length is the maximum size of cipher decoded.
    memset(cipher_decrypted, 0, sizeof(cipher_decrypted));

    // char b64decoded[1000];
    int datalen;

    aes.set_key(aesKey, 16);  // Get the globally defined key
    datalen = base64_decode(message, (char *)iv_encoded, strlen(iv_encoded));
    for (int i = 0; i < N_BLOCK; i++) {
        iv_decoded[i] = (byte)message[i];
    }

    /* 
        // Previous logic where we were encoding data twice.
        datalen = base64_decode(message,(char *)payload,strlen(payload));
        aes.do_aes_decrypt((byte *)message, datalen , cipher_decrypted, aesKey, 128, (byte *)iv_decoded);
        // Serial.println(ESP.getFreeHeap());
        datalen = base64_decode(message, (char *)cipher_decrypted, aes.get_size() );
        Serial.print("[DEBUG] Message Descrypted :  ");
        Serial.println(message);
    */

    datalen = base64_decode(message, (char *)payload, strlen(payload));
    aes.do_aes_decrypt((byte *)message, datalen + 1, cipher_decrypted, aesKey, 128, (byte *)iv_decoded);

    // datalen = base64_decode(message, (char *)cipher_decrypted, aes.get_size() );
    cipher_decrypted[aes.get_size()] = '\0';
    strcpy(message, (char *)cipher_decrypted);

    // Serial.print("[DEBUG] Message Descrypted :  ");
    // Serial.println(message);

    // Serial.println(ESP.getFreeHeap());
    // Serial.println("Cipher Decrypted B64 encoded : "+ String((char *)cipher_decrypted));
    // Serial.println(datalen);
    // Serial.println("hello");
    // yieldEspCPU();
    // ESP.wdtFeed();
    // yield();

    while (datalen--) {
        if ((byte)message[datalen] < 25) {
            message[datalen] = '\0';
        } else {
            break;
        }
    }

    // Serial.println(message);
    // DEBUG_MSG_P(PSTR("DECryption took %d Micros \n"), micros() - micros_begin);

    // DEBUG_MSG_P(PSTR("Encrypted data in base64: %s \n"), message);
    // DEBUG_MSG_P(PSTR("Encoded IV : %s \n"),iv_encoded );
    // DEBUG_MSG_P(PSTR("Decoded Message : %s \n"),message );
}
void decryptMsg(const char *payload, char *message, int payload_len, const char *iv_encoded) {
    decryptMsg(payload, message, payload_len, iv_encoded, aesKey);
}


void decodeWifiPass(const char *RevB64Pass, char *buffer, int payloadLen) {
    char localBuffer[payloadLen + 1];
    memset(localBuffer, 0, payloadLen + 1);
    for (int i = 0; i < payloadLen; i++) {
        localBuffer[i] = RevB64Pass[payloadLen - 1 - i];
    }

    base64_decode(buffer, localBuffer, payloadLen);
    return;
}

uint8_t getrnd() {
    return (uint8_t)secureRandom(255);
}

// Generate a random initialization vector
void gen_iv(byte *iv) {
    for (int i = 0; i < N_BLOCK; i++) {
        iv[i] = (byte)getrnd();
    }
}

void hmacHash(const char *message, char *output, const char *key, int key_length) {
// #if IS_GATEWAY
//     SHA256 sha256;
//     uint8_t *hash;
//     sha256.resetHMAC(key, strlen(key));
//     sha256.update(message, strlen(message));
//     sha256.finalizeHMAC(key, strlen(key), hash, sizeof(hash));
// #else
//     Sha256.initHmac((uint8_t *)key, key_length);
//     Sha256.print(message);
//     uint8_t *hash = Sha256.resultHmac();
// #endif
//     int i;
//     int j = 0;
//     for (i = 0; i < 32; i++) {
//         output[j++] = "0123456789abcdef"[hash[i] >> 4];
//         output[j++] = "0123456789abcdef"[hash[i] & 0xf];
//     }
//     output[j] = '\0';
}

bool compareHash(const char *message, const char *hash) {
    String apiSecret = getSetting("apiSecret");
    // Just for safety.
    if (apiSecret.length() == 0) {
        // setSecuritySettings();
        // saveSettings();
        return true;
    }

    char output[66] = "";
    hmacHash(message, output, apiSecret.c_str(), apiSecret.length());
    return (strcmp(output, hash) == 0);
}
