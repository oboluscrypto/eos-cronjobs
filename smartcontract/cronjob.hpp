#include <eosiolib/eosio.hpp>

#define DEBUG 0
#if DEBUG
#define D(x) x
#else
#define D(x)
#endif


using namespace eosio;

struct Cronjob
{
  name acc;
  name id;
  uint64_t repeatInS;
  name action;
};


CONTRACT cronjob : public eosio::contract {
 public:

  TABLE cronjobtable {
    Cronjob cronjob;
    uint64_t lastRun = 0;
    uint64_t primary_key()const { return cronjob.id.value; }
  };
  typedef eosio::multi_index<"cronjobs"_n, cronjobtable> cronjobs_t;
  using contract::contract;

  //constructor
  cronjob(name receiver, name code, datastream<const char*> ds);

  ACTION insertjob(name caller, name id, name action, uint64_t firstRunUTC, uint64_t repeatInS);
  ACTION runandsched(name owner, name id, uint64_t nRecure);
  ACTION rmalljobs(name owner);
  ACTION rmjob(name owner, name id);
  void updateLastRun(cronjobs_t& cronjobs, name id);

 private:
};
