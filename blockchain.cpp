using json = nlohmann::json;

using namespace std;


class block{
private:
  
  uint16_t size = 15;

public:
  vector<uint64_t>vec;
  uint64_t height;
  uint64_t previous_hash;
  uint64_t timestamp;
  vector<transaction> transactions;
  uint64_t block_reward_receive;
  PublicKey block_reward_address;
  uint64_t nonce=0;
  uint64_t nonce_2=0;
  uint64_t hash;
  
  vector<uint64_t> vectorize(){
    transaction tmp;
    vec.clear();
    vec.reserve(size);
    vec.push_back(nonce_2);
    vec.push_back(nonce);
    vec.push_back(height);
    vec.push_back(previous_hash);
    vec.push_back(timestamp);
    vector<uint64_t> v;
    v.reserve(transactions.size()*(1+5+5+8+1));
    for(uint16_t j = 0;j<transactions.size();j++){
      tmp=transactions.at(j);
      v.push_back(tmp.number);
      for(char i = 0;i<5;++i)
        v.push_back(tmp.sender.parts[i]);
      for(char i = 0;i<5;++i)
        v.push_back(tmp.receiver.parts[i]);
      v.push_back(tmp.amount);
      for(char i = 0;i<8;++i)
        v.push_back(tmp.signature[i]);
    }
    uint64_t a,b,c,d;
    tinyhash256(v,a,b,c,d);
    vec.push_back(a);
    vec.push_back(b);
    vec.push_back(c);
    vec.push_back(d);
    vec.push_back(block_reward_receive);
    for(char i = 0;i<5;++i)
      vec.push_back(block_reward_address.parts[i]);
    return vec;
  }
  void print_vectorization(const vector<uint64_t>& v) {
    cout << "Vectorizing " << v.size() << " values:\n";
    for (size_t i = 0; i < v.size() && i < 20; i++) {
        cout << "  [" << i << "] = " << v[i] << "\n";
      }
  }
  uint64_t calculate_hash(){
    vec=vectorize();
    return  tinyhash(vec);
  }
  bool isvalid(uint64_t difficulty){
    vec=vectorize();
    return (hash<difficulty)&&(calculate_hash()==hash);
  }
  void mine(uint64_t difficulty){
    bool enter=1;
    
    while(true){
      nonce = rand()*8;
      vec=vectorize();
      while(vec[1]<0xffffffffffffffffULL){
        vec[1]++;
        if(tinyhash(vec)<=difficulty) goto exit;
        if(vec[1]%32768==0)cout<<vec[1]<<endl;
      }
      nonce_2=rand();
    }
    exit:
    nonce=vec.at(1);
    hash=calculate_hash();
    
    cout<<"block mined! hash = "<<hex<<hash<<"\nnonce = "<<nonce_2<<nonce;
    print_vectorization(vec);
  }
};






class blockchain{

public:
  uint64_t height=0;
  uint64_t trans_number;
  vector<block> chain;
  vector<transaction> pending_transactions;
  uint64_t difficulty=68719476736;
  uint64_t reward=524288;
  uint64_t miners=2;
  blockchain(){
    pending_transactions.reserve(100);
    cout<<"created new blockchain!\n";
  }
  void genesisblock(uint64_t timestamp){
    block genesis;
    genesis.height=0;
    genesis.timestamp=timestamp;
    genesis.previous_hash=0;
    genesis.transactions={};
    genesis.block_reward_receive=524288;
    PublicKey key = {17696393948516182787ULL,13902621260475358775ULL,15621036470522131904ULL,9159884703671929289ULL,242ULL};
    genesis.block_reward_address=key;
    genesis.mine(difficulty);
    chain.push_back(genesis);
  }
  void add_transaction(transaction trans){
    pending_transactions.push_back(trans);
    cout<<"queue is "<<pending_transactions.size()<<" long\n";
  }
  block get_last_block(){
    return chain.back(); 
  }
  uint64_t get_balance(PublicKey address){
    //this is tricky because it requires unwinding the blockchain
    
    block b;
    transaction tr;
    uint64_t balance=0;
    int i;
    for(i = 0; i<chain.size(); i++){
      b=chain.at(i);
      vector<transaction> t=b.transactions;
      if(b.block_reward_address.parts[0]==address.parts[0]&&
        b.block_reward_address.parts[1]==address.parts[1]&&
        b.block_reward_address.parts[2]==address.parts[2]&&
        b.block_reward_address.parts[3]==address.parts[3]&&
        b.block_reward_address.parts[4]==address.parts[4]
      )
        balance+=b.block_reward_receive;
      for(int j = 0; j<t.size(); j++){
        tr=t.at(j);
        if(tr.sender.parts[0]==address.parts[0]&&
        tr.sender.parts[1]==address.parts[1]&&
        tr.sender.parts[2]==address.parts[2]&&
        tr.sender.parts[3]==address.parts[3]&&
        tr.sender.parts[4]==address.parts[4]
          ){
          if(balance < tr.amount){
            cout<<"!negative balance!"<<endl;
            return 0;
          }
          balance-=tr.amount;
        }
        if(tr.receiver.parts[0]==address.parts[0]&&
        tr.receiver.parts[1]==address.parts[1]&&
        tr.receiver.parts[2]==address.parts[2]&&
        tr.receiver.parts[3]==address.parts[3]&&
        tr.receiver.parts[4]==address.parts[4]
          ) balance+=tr.amount;
      }
    }
    return balance;
  }
  void mine_new(uint64_t timestamp, PublicKey address){
    block b,l=get_last_block();
    height++;
    b.height=l.height+1;
  
    b.timestamp=timestamp;
    b.previous_hash=l.hash;
    b.transactions=pending_transactions;
    uint64_t total_transferred=0;
    for(auto i : b.transactions)
      total_transferred+=i.amount;
    pending_transactions.clear();
    pending_transactions.reserve(100);
    b.block_reward_receive=reward+(total_transferred/1024)+1;
    b.block_reward_address=address;
    b.mine(difficulty);
    chain.push_back(b);
    if(height%131072==0&&height>0) update_difficulty();
  }
  bool send_block(block b){
    if(b.isvalid(difficulty)){
      chain.push_back(b);
      return 0;
    }
    return 1;
  }
  transaction new_transaction(PrivateKey private_key, PublicKey receiver, uint64_t amount, bool ignore){
    transaction trans;
    trans.number=time(nullptr);
    get_public_from_private(private_key, trans.sender);
    trans.receiver=receiver;
    trans.amount=amount;
    int64_t balance = get_balance(trans.sender);
    if (balance<trans.amount&&!ignore){
      cout<<"you do not have enough money "<<balance<<endl;
      trans.amount=0;
    }
    sign(trans, private_key);
    return trans;
  }
  
  void update_difficulty(){
    if(height>=131072){ reward=458752;}
    if(height>=262144){ reward=393216;}
    if(height>=393216){ reward=327680;}
    if(height>=524288){ reward=262144;}
    if(height>=655360){ reward=229376;}
    if(height>=786432){ reward=196608;}
    if(height>=917504){ reward=163840;}
    if(height>=1048576){ reward=131072;}
    if(height>=1179648){ reward=114688;}
    if(height>=1310720){ reward=98304;}
  }

  
    bool save_to_file(const string& filename) {
        json j;
        
        json chain_array = json::array();
        for (const auto& b : chain) {
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
            
            chain_array.push_back(block_json);
        }
        
        j["chain"] = chain_array;
        j["difficulty"] = difficulty;
        j["reward"] = reward;
        j["height"]=height;
        j["miners"]=miners;
        json tx_array = json::array();
        for (const auto& trans : pending_transactions) {
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
        j["pending_transactions"] = tx_array;
        
        // Write to file
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Failed to open file for writing: " << filename << endl;
            return false;
        }
        
        file << setw(4) << j << endl;
        file.close();
        
        cout << "Blockchain saved to " << filename << " (" << chain.size() 
             << " blocks, " << pending_transactions.size() << " pending tx)\n";
        
        
        return true;
    
    }
    bool load_from_file(const string& filename) {
      ifstream file(filename);
      if (!file.is_open()) {
          cout << "failed to open file: "<<filename <<endl;
          return false;
      }
        
      json j;
      file >>j;
      file.close();
      chain.clear();
      pending_transactions.clear();
        
      if (j.contains("chain") && j["chain"].is_array()) {
        for (const auto& block_json : j["chain"]) {
          block b;
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
                
          chain.push_back(b);
        }
      }
        
      difficulty = j["difficulty"];
      reward = j["reward"];
      height =j["height"];
      miners =j["miners"];
      for (const auto& tx_json : j["pending_transactions"]) {
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
        pending_transactions.push_back(trans);
      }
        
        
        return true;
        
    }

    
    bool append_block_to_file(const string& filename, const block& new_block) {
        blockchain temp;
        if (temp.load_from_file(filename)) {
            temp.chain.push_back(new_block);
            return temp.save_to_file(filename);
        }
        return 0;
    }

    bool is_chain_valid() {
      if (chain.empty()) {
        cout << "empty" << endl;
        return 0;
      }
      if (chain.at(0).previous_hash != 0) {
        cout << "invalid genesis block: previous_hash should be 0"<< endl;
        return 0;
      }
      for (size_t i = 0; i < chain.size(); i++) {
        block current = chain.at(i);
        uint64_t hash = current.hash;
        current.vec=current.vectorize();
        current.print_vectorization(current.vec);
        if (hash != tinyhash(current.vec)) {
          cout<<"block " << i << " hash mismatch! Stored: " <<dec<< hash<<" calculated: "<< current.calculate_hash()<< endl;
          return 0;
        }
        for(auto i : current.transactions){
          if(!verify(i)){
            cout<<"A TRANSACTION IS NOT SIGNED!!";
            return 0;
          }
        }
        if (i > 0) {
          if (current.previous_hash != chain.at(i-1).hash) {
            cout<< "block "<<i<<" previous hash mismatch! expected: "<< dec<< chain.at(i-1).hash <<" got: "<<current.previous_hash<< endl;
            return 0;
          }
        }
        if (hash >= difficulty) {
            cout<< "block "<<i<<" does not meet difficulty requirements! hash: "<<dec<<hash
            <<"\ndifficulty: "<<difficulty<<endl;
            return 0;
        }

      }
      cout << "validation passed " << chain.size() << "blocks are valid." << endl;
      return 1;
    }
};



