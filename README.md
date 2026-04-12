# hash64
a PoW blockchain based cryptocurrency written in C++\
\
get the latest node binaries in the release tab\
\
HTTP endpoints:\
  GET http://yourip:8080/keypair \
    create a new public/private keypair\
  GET http://yourip:8080/balance?q=public_key \
    get the balance of that wallet address\
  GET http://yourip:8080/validate_chain \
    validate current blockchain status\
  POST http://yourip:8080/new_transaction \
    send a new transaction to the server as JSON\
  POST http://yourip:8080/send_block \
    send your mined block to the server as JSON\
  GET http://yourip:8080/last_block \
    get the latest block in the blockchain\
  GET http://yourip:8080/pending_transactions \
    get the current list of pending transactions\

discuss on https://www.reddit.com/r/hash64

compile with this command: g++ -O3 -m64 blockchain_cli.cpp -o blockchain_64 -lcrypto -w\
note that openssl is required to be installed on your system

here is the genesis block:\

{\
    "chain": [\
        {\
            "block_reward_receive": 524288,\
            "extended_nonce": 0,\
            "hash": 65115539628,\
            "height": 0,\
            "nonce": 187384981,\
            "previous_hash": 0,\
            "r0": 17696393948516182787,\
            "r1": 13902621260475358775,\
            "r2": 15621036470522131904,\
            "r3": 9159884703671929289,\
            "r4": 242,\
            "timestamp": 1775995801,\
            "transactions": []\
        }\
    ],\
    "difficulty": 68719476736,
    "height": 0,
    "miners": 2,
    "pending_transactions": [],
    "reward": 524288
}

this project uses httplib.h and json.hpp, which are licensed under the MIT license
