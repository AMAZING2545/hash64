#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <unordered_map>
#include <random>
#include <algorithm>
#include "json.hpp"
#include "httplib.h"
#include <fstream>
#include <sstream>
#include "cryptography.cpp"
#include "blockchain.cpp"
bool canmine=false;
int mostFrequent(const std::vector<uint64_t>& vec) {
    if (vec.empty()) 
        return -1;
    unordered_map<uint64_t, int> frequency;
    for (const auto& num : vec)
        frequency[num]++;
    
    int maxFreq = 0;
    for (const auto& pair : frequency) {
        if (pair.second > maxFreq)
            maxFreq = pair.second;
    }

    for (size_t i = 0; i < vec.size(); ++i) {
        if (frequency[vec[i]] == maxFreq)
            return static_cast<int>(i);
    }
}

void waiting(blockchain &bc, vector<uint64_t>&hash_buffer, vector<block>&buffer, vector<int>&t, 
vector<transaction>&block_trans){
    t.reserve(10000);
    while(true){
        this_thread::sleep_for(std::chrono::milliseconds(5000));
        if(!bc.pending_transactions.empty()){
          int start_time=time(NULL);
          block_trans=bc.pending_transactions;//even who mines late gets the right transaction list
          cout<<"start mining"<<endl;
          for(auto i : block_trans)
            cout<<i.amount<<endl;
          canmine=true;
          bc.pending_transactions.clear();//who sent a transaction here gets appended to the next block
          this_thread::sleep_for(std::chrono::seconds(500)); //aiming for 8 min blocks
          while(hash_buffer.size()<2){
            this_thread::sleep_for(std::chrono::seconds(1));
          }
          canmine=false;
          int a = mostFrequent(hash_buffer);
          //first valid block
          bc.chain.push_back(buffer.at(a));
          bc.height++;
          bc.save_to_file("chain.json");
          buffer.clear();
          hash_buffer.clear();
          block_trans.clear();
          //find the median time taken to mine a block
          int median = start_time-t.at(t.size()/2);
          cout<<"old difficulty = "<<bc.difficulty<<endl;
          //if the median is greater than 500/phi+20 seconds decrease the target hash by 1/64ths
          if(median>(309+20)) bc.difficulty-=bc.difficulty/64+1;
          else if(median<(309-20)) bc.difficulty+=bc.difficulty/64+1;
          cout<<"new difficulty = "<<bc.difficulty<<endl;
          t.clear();
        }
    }
}
int main() {
    blockchain bc;
    vector<int> t;
    vector<block> buffer;
    vector<transaction> block_trans;
    vector<uint64_t> hash_buffer;
    const string save_file = "chain.json";
    
    ifstream test(save_file);
    if(test.good()) {
      test.close();
      cout << "loading\n";
      if (!bc.load_from_file(save_file)) {
        cout << "creating genesis block\n";
        bc.genesisblock(time(nullptr));
      }
    }
    else {
      cout << "creating genesis block\n";
      bc.genesisblock(time(nullptr));
    }
    
    
    httplib::Server server;
    
    server.set_keep_alive_max_count(5);
    server.set_read_timeout(5, 0);   // 5 seconds
    server.set_write_timeout(5, 0);
    server.set_idle_interval(0, 100000); // 100ms
    
    server.Get("/canmine", [&canmine]( const auto &req, auto &res){
      res.status=200;
      res.set_content(to_string(canmine), "text/plain");
    });
    
    server.Get("/", [](const auto &req, auto &res){
      res.status=200;
      string response=R"(
      <center>
      <h1>hash64 homepage</h1>
      <h3><a href="/keypair">generate new keypair(save both keys) for offline usage</a></h3>
      </center>
      <h2 style="text-align: center;"><a href="/send">make transactions online</a></h2>
      <h2 style="text-align: center;"><a href="/get_block">block explorer(use queries to navigate)</a></h2>
      )";
      
      res.set_content(response, "text/html");
    });
    
    server.Get("/send", [](const auto &req, auto &res){
      string response=R"""(
      <center>
      <h1>enter your private key(padded with zeros)</h1>
      <input type="text" id="key0">
      <h1>enter the destination key(padded with zeros)</h1>
      <input type="text" id="key1">
      amount
      <input type="number" id="amount">
      <button onclick="send()">send</button>
      <h1 id="H1"></h1>
      </center>
<script>
async function send() {
        let data = document.getElementById("key0").value;
        data+=':';
        data+=document.getElementById("key1").value;
        data+=':';
        data+=document.getElementById("amount").value;
        const response = await fetch('https://nongenuine-brittani-already.ngrok-free.dev/send/transaction',{
            method: 'POST',
            headers: {
                'Content-Type': 'text/plain',
            },
            body: data
        });
        const data1 = await response.text();
        console.log(data1);
        a=document.getElementById("H1");
        a.innerHTML = "<b>Bold Text</b>";
}
</script>
      )""";
      res.set_content(response, "text/html");
    });
    server.Post("/send/transaction", [&bc](const auto &req, auto &res) {
        string q = req.body;
        size_t first_delim = q.find(':');
        size_t second_delim = q.find(':', first_delim + 1);
    
        string priv_hex = q.substr(0, first_delim);
        string pub_hex = q.substr(first_delim + 1, second_delim - first_delim - 1);
        string am = q.substr(second_delim + 1);
    
        PrivateKey key;
        for (int i = 0; i < 4; i++) {
          string chunk = priv_hex.substr(i * 16, 16);
          key.parts[i] = stoull(chunk, nullptr, 16);
          cout<<key.parts[i]<<endl;
        }
    
        PublicKey key1;
        for (int i = 0; i < 5; i++) {
          string chunk = pub_hex.substr(i * 16, 16);
          key1.parts[i] = stoull(chunk, nullptr, 16);
          cout<<key1.parts[i]<<endl;
        }
    
        uint64_t amount = stoull(am);
        cout<<amount<<endl;
        transaction trans=bc.new_transaction(key,key1,amount,0);
        cout<<trans.amount;
        if (trans.amount==0) goto skip;
        bc.pending_transactions.push_back(trans);
        
        skip:
        res.status = 200;
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content("ok", "text/plain");
        
    });
    server.Get("/keypair", [](const auto &req, auto &res) {
          res.status = 200;
          PrivateKey pri;
          PublicKey pub;
          generate_keypair(pri, pub);
          stringstream ss;
          ss<<"private"<<"key=\n"
          <<"0:"<<hex<<pri.parts[0]<<endl
          <<"1:"<<hex<<pri.parts[1]<<endl
          <<"2:"<<hex<<pri.parts[2]<<endl
          <<"3:"<<hex<<pri.parts[3]<<endl
          <<"compressed:";
          for(int i = 0; i<4; i++){
            char buffer[16];
            sprintf(buffer,"%016llX", pri.parts[i]);
            ss<<buffer;
          }
          
          ss<<"\npublic"<<"key=\n"
          <<"0:"<<hex<<pub.parts[0]<<endl
          <<"1:"<<hex<<pub.parts[1]<<endl
          <<"2:"<<hex<<pub.parts[2]<<endl
          <<"3:"<<hex<<pub.parts[3]<<endl
          <<"4:"<<hex<<pub.parts[4]<<endl
          <<"compressed:";
          for(int i = 0; i<5; i++){
            char buffer[16];
            sprintf(buffer,"%016llX", pub.parts[i]);
            ss<<buffer;
          }
          string response=ss.str();
          res.set_content(response, "text/plain");
    });
    server.Get("/validate_chain", [&bc](const auto &req, auto &res) {
          res.status = 200;
          string response;
          if(bc.is_chain_valid())
            response="valid";
          else
            response="invalid";
          res.set_content(response, "text/plain");
    });
server.Get("/balance", [&bc](const auto &req, auto &res) {
        string q = req.get_param_value("q");
        if (q.length() < 80) {
            res.status = 400;
            res.set_content("Invalid public key length", "text/plain");
            return;
        }
        PublicKey key;
        memset(key.parts, 0, sizeof(key.parts));
        for (int i = 0; i < 5; i++) {
            string chunk = q.substr(i * 16, 16);
            key.parts[i] = stoull(chunk, nullptr, 16);
        }
        uint64_t balance = bc.get_balance(key);
        string response = "balance = " + to_string(balance);
        res.status = 200;
        res.set_content(response, "text/plain");
        
});
    server.Post("/new_transaction", [&bc](const auto &req, auto &res) {
          json tx_json = json::parse(req.body);
                transaction trans;
                trans.number = tx_json["number"];
                trans.sender.parts[0] = tx_json["s0"];
                trans.sender.parts[1] = tx_json["s1"];
                trans.sender.parts[2] = tx_json["s2"];
                trans.sender.parts[3] = tx_json["s3"];
                trans.sender.parts[4] = tx_json["s4"];
                trans.receiver.parts[0] = tx_json["r0"];
                trans.receiver.parts[1] = tx_json["r1"];
                trans.receiver.parts[2] = tx_json["r2"];
                trans.receiver.parts[3] = tx_json["r3"];
                trans.receiver.parts[4] = tx_json["r4"];
                trans.amount = tx_json["amount"];
                trans.signature[0] = tx_json["sig0"];
                trans.signature[1] = tx_json["sig1"];
                trans.signature[2] = tx_json["sig2"];
                trans.signature[3] = tx_json["sig3"];
                trans.signature[4] = tx_json["sig4"];
                trans.signature[5] = tx_json["sig5"];
                trans.signature[6] = tx_json["sig6"];
                trans.signature[7] = tx_json["sig7"];
          string response;

          bc.get_balance(trans.sender);
          if(!verify(trans))
            response="invalid transaction";
          else{
            response="transaction submitted, waiting for approval";
            bc.pending_transactions.push_back(trans);
          }
          res.status = 200;
          res.set_header("Access-Control-Allow-Origin", "*");
          res.set_content(response, "text/plain");
    });
    server.Post("/send_block", [&buffer, &hash_buffer, &bc, &t](const auto &req, auto &res) {
          json block_json=json::parse(req.body);
          block b;
          cout<<"hello";
          b.previous_hash = block_json["previous_hash"];
          b.timestamp = block_json["timestamp"];
          b.block_reward_receive = block_json["block_reward_receive"];
          b.block_reward_address.parts[0]=block_json["r0"];
          b.block_reward_address.parts[1]=block_json["r1"];
          b.block_reward_address.parts[2]=block_json["r2"];
          b.block_reward_address.parts[3]=block_json["r3"];
          b.block_reward_address.parts[4]=block_json["r4"];
          b.hash = block_json["hash"];
          b.height=block_json["height"];
          b.nonce=block_json["nonce"];
          b.nonce_2=block_json["extended_nonce"];
          if (block_json.contains("transactions")&&block_json["transactions"].is_array()) {
              for (const auto& tx_json : block_json["transactions"]) {
                transaction trans;
                trans.number = tx_json["number"];
                trans.sender.parts[0] = tx_json["s0"];
                trans.sender.parts[1] = tx_json["s1"];
                trans.sender.parts[2] = tx_json["s2"];
                trans.sender.parts[3] = tx_json["s3"];
                trans.sender.parts[4] = tx_json["s4"];
                trans.receiver.parts[0] = tx_json["r0"];
                trans.receiver.parts[1] = tx_json["r1"];
                trans.receiver.parts[2] = tx_json["r2"];
                trans.receiver.parts[3] = tx_json["r3"];
                trans.receiver.parts[4] = tx_json["r4"];
                trans.amount = tx_json["amount"];
                trans.signature[0] = tx_json["sig0"];
                trans.signature[1] = tx_json["sig1"];
                trans.signature[2] = tx_json["sig2"];
                trans.signature[3] = tx_json["sig3"];
                trans.signature[4] = tx_json["sig4"];
                trans.signature[5] = tx_json["sig5"];
                trans.signature[6] = tx_json["sig6"];
                trans.signature[7] = tx_json["sig7"];
                b.transactions.push_back(trans);
              }
          }
          string response;
          b.print_vectorization(b.vectorize());
          cout<<dec<<tinyhash(b.vectorize())<<flush;
          bool valid=tinyhash(b.vectorize())==b.hash;
          for(auto i : b.transactions){
              if (!verify(i)){
                valid = 0;
                cout<<"invalid transaction";
                break;
              }
          }
          if(!valid){
             response = "INVALID BLOCK, rejected";
          }
          else{
            response = "VALID, waiting for verification";
            buffer.push_back(b);
            uint64_t tmp_time = b.timestamp, tmp_nonce=b.nonce, tmp_exnonce=b.nonce_2;
            PublicKey tmp_key = b.block_reward_address;
            b.timestamp=0;
            b.nonce=0;
            b.nonce_2=0;
            b.block_reward_address={0,0,0,0,0};
            vector<uint64_t> vc;
            vc=b.vectorize();
            hash_buffer.push_back(tinyhash(vc));
            b.timestamp=tmp_time;
            b.nonce=tmp_nonce;
            b.nonce_2=tmp_exnonce;
            b.block_reward_address=tmp_key;
            t.push_back(time(nullptr));
          }
        
          res.status = 200;
          res.set_header("Access-Control-Allow-Origin", "*");
          res.set_content(response, "text/plain");
    });
    
    server.Get("/last_block", [&bc](const auto &req, auto &res) {
            block b =bc.chain.back();
            cout<<"hello?"<<endl<<flush;
            string response;
            stringstream ss;
            json block_json;
            block_json["previous_hash"] = b.previous_hash;
            block_json["timestamp"] = b.timestamp;
            block_json["block_reward_receive"] = b.block_reward_receive;
            
            block_json["r0"] = b.block_reward_address.parts[0];
            block_json["r1"] = b.block_reward_address.parts[1];
            block_json["r2"] = b.block_reward_address.parts[2];
            block_json["r3"] = b.block_reward_address.parts[3];
            block_json["r4"] = b.block_reward_address.parts[4];
                
            block_json["nonce"] = b.nonce;
            block_json["extended_nonce"] = b.nonce_2;
            block_json["hash"] = b.hash;
            block_json["height"] = b.height;
            json tx_array = json::array();
            for (const auto& trans : b.transactions) {
                json tx_json;
                tx_json["number"] = trans.number;
                tx_json["s0"] = trans.sender.parts[0];
                tx_json["s1"] = trans.sender.parts[1];
                tx_json["s2"] = trans.sender.parts[2];
                tx_json["s3"] = trans.sender.parts[3];
                tx_json["s4"] = trans.sender.parts[4];
                tx_json["r0"] = trans.receiver.parts[0];
                tx_json["r1"] = trans.receiver.parts[1];
                tx_json["r2"] = trans.receiver.parts[2];
                tx_json["r3"] = trans.receiver.parts[3];
                tx_json["r4"] = trans.receiver.parts[4];
                tx_json["amount"] = trans.amount;
                tx_json["sig0"] = trans.signature[0];
                tx_json["sig1"] = trans.signature[1];
                tx_json["sig2"] = trans.signature[2];
                tx_json["sig3"] = trans.signature[3];
                tx_json["sig4"] = trans.signature[4];
                tx_json["sig5"] = trans.signature[5];
                tx_json["sig6"] = trans.signature[6];
                tx_json["sig7"] = trans.signature[7];
                tx_array.push_back(tx_json);
            }
            block_json["transactions"] = tx_array;
            ss<<block_json;
            ss>>response;
            cout<<response<<endl;
            res.status = 200;
          res.set_content(response, "text/plain");
    });
      server.Get("/get_block", [&bc](const auto &req, auto &res) {
            uint64_t i = stoi(req.get_param_value("q"));
            block b =bc.chain.at(i);
            cout<<"hello?"<<endl<<flush;
            string response;
            stringstream ss;
            json block_json;
            block_json["previous_hash"] = b.previous_hash;
            block_json["timestamp"] = b.timestamp;
            block_json["block_reward_receive"] = b.block_reward_receive;
            
            block_json["r0"] = b.block_reward_address.parts[0];
            block_json["r1"] = b.block_reward_address.parts[1];
            block_json["r2"] = b.block_reward_address.parts[2];
            block_json["r3"] = b.block_reward_address.parts[3];
            block_json["r4"] = b.block_reward_address.parts[4];
                
            block_json["nonce"] = b.nonce;
            block_json["extended_nonce"] = b.nonce_2;
            block_json["hash"] = b.hash;
            block_json["height"] = b.height;
            json tx_array = json::array();
            for (const auto& trans : b.transactions) {
                json tx_json;
                tx_json["number"] = trans.number;
                tx_json["s0"] = trans.sender.parts[0];
                tx_json["s1"] = trans.sender.parts[1];
                tx_json["s2"] = trans.sender.parts[2];
                tx_json["s3"] = trans.sender.parts[3];
                tx_json["s4"] = trans.sender.parts[4];
                tx_json["r0"] = trans.receiver.parts[0];
                tx_json["r1"] = trans.receiver.parts[1];
                tx_json["r2"] = trans.receiver.parts[2];
                tx_json["r3"] = trans.receiver.parts[3];
                tx_json["r4"] = trans.receiver.parts[4];
                tx_json["amount"] = trans.amount;
                tx_json["sig0"] = trans.signature[0];
                tx_json["sig1"] = trans.signature[1];
                tx_json["sig2"] = trans.signature[2];
                tx_json["sig3"] = trans.signature[3];
                tx_json["sig4"] = trans.signature[4];
                tx_json["sig5"] = trans.signature[5];
                tx_json["sig6"] = trans.signature[6];
                tx_json["sig7"] = trans.signature[7];
                tx_array.push_back(tx_json);
            }
            block_json["transactions"] = tx_array;
            ss<<block_json;
            ss>>response;
            cout<<response<<endl;
            res.status = 200;
          res.set_content(response, "text/plain");
    });
    server.Get("/pending_transactions", [&bc](const auto &req, auto &res) {

            string response;
            stringstream ss;
            json block_json;
            json tx_array = json::array();
            for (const auto& trans : bc.pending_transactions) {
                json tx_json;
                tx_json["number"] = trans.number;
                tx_json["s0"] = trans.sender.parts[0];
                tx_json["s1"] = trans.sender.parts[1];
                tx_json["s2"] = trans.sender.parts[2];
                tx_json["s3"] = trans.sender.parts[3];
                tx_json["s4"] = trans.sender.parts[4];
                tx_json["r0"] = trans.receiver.parts[0];
                tx_json["r1"] = trans.receiver.parts[1];
                tx_json["r2"] = trans.receiver.parts[2];
                tx_json["r3"] = trans.receiver.parts[3];
                tx_json["r4"] = trans.receiver.parts[4];
                tx_json["amount"] = trans.amount;
                tx_json["sig0"] = trans.signature[0];
                tx_json["sig1"] = trans.signature[1];
                tx_json["sig2"] = trans.signature[2];
                tx_json["sig3"] = trans.signature[3];
                tx_json["sig4"] = trans.signature[4];
                tx_json["sig5"] = trans.signature[5];
                tx_json["sig6"] = trans.signature[6];
                tx_json["sig7"] = trans.signature[7];
                tx_array.push_back(tx_json);
            }

            ss<<tx_array;
            ss>>response;
            cout<<response<<endl;
            res.status = 200;
          res.set_content(response, "text/plain");
    });
        server.Get("/block_transactions", [&block_trans, &bc](const auto &req, auto &res) {
            string response;
            stringstream ss;
            json j;
            json tx_array = json::array();
            for (const auto& trans : block_trans) {
                json tx_json;
                tx_json["number"] = trans.number;
                tx_json["s0"] = trans.sender.parts[0];
                tx_json["s1"] = trans.sender.parts[1];
                tx_json["s2"] = trans.sender.parts[2];
                tx_json["s3"] = trans.sender.parts[3];
                tx_json["s4"] = trans.sender.parts[4];
                tx_json["r0"] = trans.receiver.parts[0];
                tx_json["r1"] = trans.receiver.parts[1];
                tx_json["r2"] = trans.receiver.parts[2];
                tx_json["r3"] = trans.receiver.parts[3];
                tx_json["r4"] = trans.receiver.parts[4];
                tx_json["amount"] = trans.amount;
                tx_json["sig0"] = trans.signature[0];
                tx_json["sig1"] = trans.signature[1];
                tx_json["sig2"] = trans.signature[2];
                tx_json["sig3"] = trans.signature[3];
                tx_json["sig4"] = trans.signature[4];
                tx_json["sig5"] = trans.signature[5];
                tx_json["sig6"] = trans.signature[6];
                tx_json["sig7"] = trans.signature[7];
                tx_array.push_back(tx_json);
            }
            j["t"]=tx_array;
            j["d"]=bc.difficulty;
            ss<<j;
            ss>>response;
            cout<<response<<endl;
            res.status = 200;
          res.set_content(response, "text/plain");
    });
    thread server_thread([&server]() {
        server.listen("0.0.0.0", 8081);
    });
    thread approve([&](){waiting(bc,hash_buffer,buffer,t,block_trans);});
    
    int option=0;
    while(true){
      cout<<"\nblockchain CLI\n"
      <<"options:\n"
      <<"1: append transaction (requires private key)\n"
      <<"2: check balance\n"
      <<"3: create new wallet\n"
      <<"4: save chain\n"
      <<"5: mine latest block\n"
      <<"6: make new transaction as JSON\n"
      <<"7: make new transaction as JSON(ignore balance)\n"
      <<"8: get last block as JSON\n"
      <<"9: append a block as JSON\n"
      <<"10: verify chain\n"
      <<"11: override current transaction list with JSON\n";
      cin>>option;
      
      switch(option){
        case 1:{
          uint64_t amount;
          PrivateKey private_key;
          PublicKey destination;
          cout<<"enter your private key(it won't be sent to the network)";
          for(int i = 0; i<4; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>private_key.parts[i];
          }
          cout<<"enter amount ";
          cin>>dec>>amount;
          cout<<"enter recipient's public key ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>destination.parts[i];
          }
          bc.add_transaction(bc.new_transaction(private_key,destination,amount,0));
          bc.trans_number++;
          break;
        }
        case 2:{
          PublicKey key;
          cout<<"enter address ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>key.parts[i];
          }

          cout<<dec<<"coins = "<<bc.get_balance(key)<<endl;
          break;
        }
        case 3:{
          PrivateKey priv_;
          PublicKey pub;
          generate_keypair(priv_, pub);
          cout<<"Private key(write this down on a piece of paper)\n";
          for(int i = 0; i<4; i++){
            cout<<i<<"    "<<hex<<priv_.parts[i]<<endl;
          }
          cout<<"Public key\n";
          for(int i = 0; i<5; i++){
            cout<<i<<"    "<<hex<<pub.parts[i]<<endl;
          }
          break;
        }
        case 4:{
          string fil;
          cout<<"enter filename ";
          cin>>fil;
          bc.save_to_file(fil);
          break;
        }
        case 5:{
          PublicKey addr;
          cout<<"enter reward address ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>addr.parts[i];
          }
          cout<<"mining block "<<dec<<bc.height;
          bc.mine_new(time(nullptr),addr);
          break;
        }
        case 10 :{
          if(bc.is_chain_valid()) cout<<"valid";
          break;
        }
        case 6:{
          uint64_t amount;
          PrivateKey private_key;
          PublicKey destination;
          cout<<"enter your private key(it won't be sent to the network)";
          for(int i = 0; i<4; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>private_key.parts[i];
          }
          cout<<"enter amount ";
          cin>>dec>>amount;
          cout<<"enter recipient's public key ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>destination.parts[i];
          }
          transaction trans = bc.new_transaction(private_key,destination,amount,0);
                json tx_json;
                tx_json["number"] = trans.number;
                tx_json["s0"] = trans.sender.parts[0];
                tx_json["s1"] = trans.sender.parts[1];
                tx_json["s2"] = trans.sender.parts[2];
                tx_json["s3"] = trans.sender.parts[3];
                tx_json["s4"] = trans.sender.parts[4];
                tx_json["r0"] = trans.receiver.parts[0];
                tx_json["r1"] = trans.receiver.parts[1];
                tx_json["r2"] = trans.receiver.parts[2];
                tx_json["r3"] = trans.receiver.parts[3];
                tx_json["r4"] = trans.receiver.parts[4];
                tx_json["amount"] = trans.amount;
                tx_json["sig0"] = trans.signature[0];
                tx_json["sig1"] = trans.signature[1];
                tx_json["sig2"] = trans.signature[2];
                tx_json["sig3"] = trans.signature[3];
                tx_json["sig4"] = trans.signature[4];
                tx_json["sig5"] = trans.signature[5];
                tx_json["sig6"] = trans.signature[6];
                tx_json["sig7"] = trans.signature[7];
          cout<<tx_json;
          break;
        }
        case 7:{
          uint64_t amount;
          PrivateKey private_key;
          PublicKey destination;
          cout<<"enter your private key(it won't be sent to the network)";
          for(int i = 0; i<4; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>private_key.parts[i];
          }
          cout<<"enter amount ";
          cin>>dec>>amount;
          cout<<"enter recipient's public key ";
          for(int i = 0; i<5; i++){
            cout<<endl<<i<<": ";
            cin>>hex>>destination.parts[i];
          }
          transaction trans = bc.new_transaction(private_key,destination,amount,1);
                json tx_json;
                tx_json["number"] = trans.number;
                tx_json["s0"] = trans.sender.parts[0];
                tx_json["s1"] = trans.sender.parts[1];
                tx_json["s2"] = trans.sender.parts[2];
                tx_json["s3"] = trans.sender.parts[3];
                tx_json["s4"] = trans.sender.parts[4];
                tx_json["r0"] = trans.receiver.parts[0];
                tx_json["r1"] = trans.receiver.parts[1];
                tx_json["r2"] = trans.receiver.parts[2];
                tx_json["r3"] = trans.receiver.parts[3];
                tx_json["r4"] = trans.receiver.parts[4];
                tx_json["amount"] = trans.amount;
                tx_json["sig0"] = trans.signature[0];
                tx_json["sig1"] = trans.signature[1];
                tx_json["sig2"] = trans.signature[2];
                tx_json["sig3"] = trans.signature[3];
                tx_json["sig4"] = trans.signature[4];
                tx_json["sig5"] = trans.signature[5];
                tx_json["sig6"] = trans.signature[6];
                tx_json["sig7"] = trans.signature[7];
          cout<<tx_json;
          break;
        }
        case 8:{
            block b =bc.get_last_block();
            json block_json;
            block_json["previous_hash"] = b.previous_hash;
            block_json["timestamp"] = b.timestamp;
            block_json["block_reward_receive"] = b.block_reward_receive;
            
            block_json["r0"] = b.block_reward_address.parts[0];
            block_json["r1"] = b.block_reward_address.parts[1];
            block_json["r2"] = b.block_reward_address.parts[2];
            block_json["r3"] = b.block_reward_address.parts[3];
            block_json["r4"] = b.block_reward_address.parts[4];
                
            block_json["nonce"] = b.nonce;
            block_json["extended_nonce"] = b.nonce_2;
            block_json["hash"] = b.hash;
            block_json["height"] = b.height;
            json tx_array = json::array();
            for (const auto& trans : b.transactions) {
                json tx_json;
                tx_json["number"] = trans.number;
                tx_json["s0"] = trans.sender.parts[0];
                tx_json["s1"] = trans.sender.parts[1];
                tx_json["s2"] = trans.sender.parts[2];
                tx_json["s3"] = trans.sender.parts[3];
                tx_json["s4"] = trans.sender.parts[4];
                tx_json["r0"] = trans.receiver.parts[0];
                tx_json["r1"] = trans.receiver.parts[1];
                tx_json["r2"] = trans.receiver.parts[2];
                tx_json["r3"] = trans.receiver.parts[3];
                tx_json["r4"] = trans.receiver.parts[4];
                tx_json["amount"] = trans.amount;
                tx_json["sig0"] = trans.signature[0];
                tx_json["sig1"] = trans.signature[1];
                tx_json["sig2"] = trans.signature[2];
                tx_json["sig3"] = trans.signature[3];
                tx_json["sig4"] = trans.signature[4];
                tx_json["sig5"] = trans.signature[5];
                tx_json["sig6"] = trans.signature[6];
                tx_json["sig7"] = trans.signature[7];
                tx_array.push_back(tx_json);
            }
            block_json["transactions"] = tx_array;
            cout<<block_json;
            break;
      }
      case 9:{
          block b;
          json block_json;
          cin>>block_json;
          b.previous_hash = block_json["previous_hash"];
          b.timestamp = block_json["timestamp"];
          b.block_reward_receive = block_json["block_reward_receive"];
          b.block_reward_address.parts[0]=block_json["r0"];
          b.block_reward_address.parts[1]=block_json["r1"];
          b.block_reward_address.parts[2]=block_json["r2"];
          b.block_reward_address.parts[3]=block_json["r3"];
          b.block_reward_address.parts[4]=block_json["r4"];
          b.hash = block_json["hash"];
          b.height=block_json["height"];
          b.nonce=block_json["nonce"];
          b.nonce_2=block_json["extended_nonce"];
          if (block_json.contains("transactions")&&block_json["transactions"].is_array()) {
              for (const auto& tx_json : block_json["transactions"]) {
                transaction trans;
                trans.number = tx_json["number"];
                trans.sender.parts[0] = tx_json["s0"];
                trans.sender.parts[1] = tx_json["s0"];
                trans.sender.parts[2] = tx_json["s0"];
                trans.sender.parts[3] = tx_json["s0"];
                trans.sender.parts[4] = tx_json["s0"];
                trans.receiver.parts[0] = tx_json["r0"];
                trans.receiver.parts[1] = tx_json["r1"];
                trans.receiver.parts[2] = tx_json["r2"];
                trans.receiver.parts[3] = tx_json["r3"];
                trans.receiver.parts[4] = tx_json["r4"];
                trans.amount = tx_json["amount"];
                trans.signature[0] = tx_json["sig0"];
                trans.signature[1] = tx_json["sig1"];
                trans.signature[2] = tx_json["sig2"];
                trans.signature[3] = tx_json["sig3"];
                trans.signature[4] = tx_json["sig4"];
                trans.signature[5] = tx_json["sig5"];
                trans.signature[6] = tx_json["sig6"];
                trans.signature[7] = tx_json["sig7"];
                b.transactions.push_back(trans);
              }
          }
          cout<<bc.send_block(b);
          break;
      }
      case 11:{
        json j;
        cin>>j;
        vector<transaction> pending;
        pending.reserve(100);
        for (const auto& tx_json : j) {
                transaction trans;
                trans.number = tx_json["number"];
                trans.sender.parts[0] = tx_json["s0"];
                trans.sender.parts[1] = tx_json["s1"];
                trans.sender.parts[2] = tx_json["s2"];
                trans.sender.parts[3] = tx_json["s3"];
                trans.sender.parts[4] = tx_json["s4"];
                trans.receiver.parts[0] = tx_json["r0"];
                trans.receiver.parts[1] = tx_json["r1"];
                trans.receiver.parts[2] = tx_json["r2"];
                trans.receiver.parts[3] = tx_json["r3"];
                trans.receiver.parts[4] = tx_json["r4"];
                trans.amount = tx_json["amount"];
                trans.signature[0] = tx_json["sig0"];
                trans.signature[1] = tx_json["sig1"];
                trans.signature[2] = tx_json["sig2"];
                trans.signature[3] = tx_json["sig3"];
                trans.signature[4] = tx_json["sig4"];
                trans.signature[5] = tx_json["sig5"];
                trans.signature[6] = tx_json["sig6"];
                trans.signature[7] = tx_json["sig7"];
                pending.push_back(trans);
        }
        bc.pending_transactions=pending;
        break;
      }
    }
    }
    return 0;
}
