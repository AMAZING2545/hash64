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

this project uses httplib.h and json.hpp, which are licensed under the MIT license
