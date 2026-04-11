#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>
#include <openssl/bn.h>

using namespace std;

struct PublicKey {
    uint64_t parts[5];  
};

struct PrivateKey {
    uint64_t parts[4];  // 256 bits = 4 * 64 bits
};

void bn_to_u64_array_le(const BIGNUM* bn, uint64_t* arr, int num_words) {
    unsigned char buffer[32];
    memset(buffer, 0, 32);
    int size = BN_bn2bin(bn, buffer);
    memset(arr, 0, num_words * sizeof(uint64_t));
    for (int i = 0; i < size; i++) {
        arr[i / 8] |= ((uint64_t)buffer[i]) << ((i % 8) * 8);
    }
}

BIGNUM* u64_array_to_bn_le(const uint64_t* arr, int num_words) {
    unsigned char buffer[32];
    memset(buffer, 0, 32);
    for (int i = 0; i < 32; i++) {
        buffer[i] = (arr[i/8] >> ((i%8)*8)) & 0xFF;
    }
    return BN_bin2bn(buffer, 32, NULL);
}
void generate_keypair(PrivateKey& priv, PublicKey& pub) {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);
    EC_KEY_generate_key(key);
    const BIGNUM* priv_bn = EC_KEY_get0_private_key(key);
    bn_to_u64_array_le(priv_bn, priv.parts, 4);

    const EC_GROUP* group = EC_KEY_get0_group(key);
    const EC_POINT* pub_point = EC_KEY_get0_public_key(key);
    
    unsigned char pub_bytes[33];
    size_t pub_len = EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_COMPRESSED,pub_bytes, 33, NULL);
    memset(pub.parts, 0, sizeof(pub.parts));
    for (int i = 0; i < 33; i++) 
            pub.parts[i / 8] |= ((uint64_t)pub_bytes[i]) << ((i % 8) * 8);
    EC_KEY_free(key);
}

void get_public_from_private(const PrivateKey& priv, PublicKey& pub) {
    BIGNUM* priv_bn = u64_array_to_bn_le(priv.parts, 4);
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    EC_KEY_set_private_key(key, priv_bn);
    const EC_GROUP* group = EC_KEY_get0_group(key);
    EC_POINT* pub_point = EC_POINT_new(group);
    EC_POINT_mul(group, pub_point, priv_bn, NULL, NULL, NULL);
    unsigned char pub_bytes[33];
    EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_COMPRESSED,pub_bytes, 33, NULL);
    
    memset(pub.parts, 0, sizeof(pub.parts));
    for (int i = 0; i < 33; i++) 
        pub.parts[i / 8] |= ((uint64_t)pub_bytes[i]) << ((i % 8) * 8);
    
    EC_POINT_free(pub_point);
    BN_free(priv_bn);
    EC_KEY_free(key);
}

void sign_hash(const uint64_t* hash256,const PrivateKey& priv,uint64_t* signature_out) {
    unsigned char hash[32];
    memset(hash, 0, 32);
    for (int i = 0; i < 32; i++) {
        hash[i] = (hash256[i / 8] >> ((i % 8) * 8)) & 0xFF;
    }

    BIGNUM* priv_bn = u64_array_to_bn_le(priv.parts, 4);
    
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    EC_KEY_set_private_key(key, priv_bn);
    
    const EC_GROUP* group = EC_KEY_get0_group(key);
    EC_POINT* pub_point = EC_POINT_new(group);
    EC_POINT_mul(group, pub_point, priv_bn, NULL, NULL, NULL);
    EC_KEY_set_public_key(key, pub_point);

    ECDSA_SIG* signature = ECDSA_do_sign(hash, 32, key);
 
    const BIGNUM* r = ECDSA_SIG_get0_r(signature);
    const BIGNUM* s = ECDSA_SIG_get0_s(signature);
    unsigned char r_bytes[32], s_bytes[32];
    memset(r_bytes, 0, 32);
    memset(s_bytes, 0, 32);
    BN_bn2bin(r, r_bytes);
    BN_bn2bin(s, s_bytes);
    
    memset(signature_out, 0, 8 * sizeof(uint64_t));
    for (int i = 0; i < 32; i++) {
        signature_out[i / 8] |= ((uint64_t)r_bytes[i]) << ((i % 8) * 8);
        signature_out[4 + i / 8] |= ((uint64_t)s_bytes[i]) << ((i % 8) * 8);
    }

    EC_POINT_free(pub_point);
    BN_free(priv_bn);
    ECDSA_SIG_free(signature);
    EC_KEY_free(key);
}

bool verify_signature(const uint64_t* hash256,const uint64_t* signature_parts, const PublicKey& pub) {
    unsigned char hash[32];
    memset(hash, 0, 32);
    for (int i = 0; i < 32; i++) hash[i] = (hash256[i / 8] >> ((i % 8) * 8)) & 0xFF;
    unsigned char pub_bytes[33];
    for (int i = 0; i < 33; i++) pub_bytes[i] = (pub.parts[i / 8] >> ((i % 8) * 8)) & 0xFF;
    if (pub_bytes[0]!=2&&pub_bytes[0]!=3) return 0;
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    const EC_GROUP* group = EC_KEY_get0_group(key);
    EC_POINT* pub_point = EC_POINT_new(group);
    
    if (!EC_POINT_oct2point(group, pub_point, pub_bytes, 33, NULL)) {
        cerr << "Failed to decode public key" << endl;
        EC_POINT_free(pub_point);
        EC_KEY_free(key);
        return 0;
    }
    EC_KEY_set_public_key(key, pub_point);
    unsigned char r_bytes[32], s_bytes[32];
    memset(r_bytes, 0, 32);
    memset(s_bytes, 0, 32);
    
    for (int i = 0; i < 32; i++) {
        r_bytes[i] = (signature_parts[i / 8] >> ((i % 8) * 8)) & 0xFF;
        s_bytes[i] = (signature_parts[4 + i / 8] >> ((i % 8) * 8)) & 0xFF;
    }
    
    BIGNUM* r = BN_bin2bn(r_bytes, 32, NULL);
    BIGNUM* s = BN_bin2bn(s_bytes, 32, NULL);
    
    ECDSA_SIG* signature = ECDSA_SIG_new();
    ECDSA_SIG_set0(signature, r, s);
    int result = ECDSA_do_verify(hash, 32, signature, key);
    ECDSA_SIG_free(signature);
    EC_POINT_free(pub_point);
    EC_KEY_free(key);
    
    return result==1;
}

void public_key_to_bytes(const PublicKey& pub, unsigned char* bytes) {
    for (int i = 0; i < 33; i++) {
        bytes[i] = (pub.parts[i / 8] >> ((i % 8) * 8)) & 0xFF;
    }
}


void tinyhash256(const vector<uint64_t>& v, uint64_t &a,
uint64_t    &b,
uint64_t    &c,
uint64_t    &d) {
    const uint64_t pi = 0x3243F6A8885A308D;
    const uint64_t euler = 0x2B7E151628AE06EE;
    const uint64_t phi = 0x19E3779B97F4A7C1;
    const uint64_t c101 = 0xDC743EE6B2D64695; //cube root of 101(prime)
    
    uint64_t state0 = pi,
    state1 = euler,
    state2 = phi,
    state3 = c101;
    uint64_t round_robin = pi*euler*phi*c101;
    
    for(auto &i : v){
      //three rounds per element
      //round 1
      state0 = state0 xor ((state0<<3) * (i+state1)) + state1 + round_robin;
      state1 = state1 xor ((state1<<5) * (i+state2)) + state3 - round_robin;
      state2 = state2 xor ((state2<<7) * (i+state3)) + state0 - round_robin;
      state3 = state3 xor ((state3<<11) * (i+state0)) + state2+ round_robin;
      round_robin = (state0*state1*state2*state3*state0+state1+state2+state3)+round_robin;  
      //round 2
      state0 = ((c101 * state2)%(state1|1)) + state0 * (state1>>3) + i*state3 + round_robin;
      state1 = ((euler * state3)%(state2|1)) + state1 * (state3>>5) + i*state0 * round_robin;
      state2 = ((phi * state0)%(state3|1)) + state2 * (state0>>7) + i*state1 + round_robin;
      state3 = ((pi * state1)%(state0|1)) + state3 * (state2>>11) + i*state2 * round_robin;
      //round 3: mixup
      round_robin = (state0*state1*state2*state3)%((state0+state1+state2+state3)|1)-round_robin;     
      state0 = (((state0 xor phi)*euler)%(state1|1)) xor pi*state3 + round_robin*state0;
      state1 = (((state1 xor c101)*phi)%(state3|1)) xor c101*state2 - round_robin*state1;
      state2 = (((state2 xor euler)*pi)%(state0|1)) xor euler*state0 + round_robin*state2;
      state3 = (((state3 xor pi)*c101)%(state2|1)) xor phi*state1 - round_robin*state3;
      round_robin = (state0*state1*state2*state3)xor(state0+state1+state2+state3)xor round_robin;
    }
    state0 = (((state0 xor phi)*euler)%(state1|1)) xor pi*2; //multiply by prime to avoid bias in the upper bits
    state1 = (((state1 xor pi)*phi)%(state3|1)) xor euler*5;
    state2 = (((state2 xor c101)*pi)%(state0|1)) xor c101;
    state3 = (((state3 xor pi)*euler)%(state1|1)) xor phi*3;
    
    //final hash
    a=state0;
    b=state1;
    c=state2;
    d=state3;
}

uint64_t tinyhash(const vector<uint64_t>& v){
  uint64_t a,b,c,d;
  tinyhash256(v,a,b,c,d);
  return a^b^c^d;
}
struct transaction {
    uint64_t number;
    PublicKey sender;
    PublicKey receiver;
    uint64_t amount;
    uint64_t signature[8];
};

// Sign a transaction
void sign(transaction& tx, const PrivateKey& priv) {
    vector<uint64_t> v={tx.number, tx.amount,
    tx.sender.parts[0],tx.sender.parts[1],tx.sender.parts[2],tx.sender.parts[3],tx.sender.parts[4],
    tx.receiver.parts[0],tx.receiver.parts[1],tx.receiver.parts[2],tx.receiver.parts[3],tx.receiver.parts[4],
    };
    uint64_t a,b,c,d;
    tinyhash256(v,a,b,c,d);
    uint64_t tx_hash_parts[4] = {a,b,c,d};
    sign_hash(tx_hash_parts, priv, tx.signature);
}

bool verify(const transaction& tx) {
    vector<uint64_t> v={tx.number, tx.amount,
    tx.sender.parts[0],tx.sender.parts[1],tx.sender.parts[2],tx.sender.parts[3],tx.sender.parts[4],
    tx.receiver.parts[0],tx.receiver.parts[1],tx.receiver.parts[2],tx.receiver.parts[3],tx.receiver.parts[4],
    };
    uint64_t a,b,c,d;
    tinyhash256(v,a,b,c,d);
    uint64_t tx_hash_parts[4] = {a,b,c,d};
    return verify_signature(tx_hash_parts, tx.signature, tx.sender);
}

void print_private_key(const PrivateKey& priv) {
    cout<<hex;
    for (int i=0;i<4;i++)
        cout << setfill('0') << setw(16) << priv.parts[i] << " ";
    cout<<dec<<endl;
}
void print_public_key(const PublicKey& pub) {
    for (int i = 0; i < 5; i++) {
        cout << hex << setfill('0') << setw(16) << pub.parts[i] << " ";
    }
    cout << dec << endl;
}
