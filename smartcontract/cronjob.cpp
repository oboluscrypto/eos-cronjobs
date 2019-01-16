#include "cronjob.hpp"
#include <eosiolib/transaction.hpp>
using namespace eosio;
using namespace std;

cronjob::cronjob(name receiver, name code, datastream<const char*> ds) : eosio::contract(receiver, code, ds)
{
}


ACTION cronjob::insertjob(name owner, name id, name actionname, uint64_t firstRunUTC, uint64_t repeatInS)
{
  require_auth(owner);
  cronjobs_t cronjobs(_self, owner.value);

  auto jobIt = cronjobs.find(id.value);
  eosio_assert(jobIt == cronjobs.end(), "Delete old job first");

  auto cronjob = Cronjob{
    .acc = owner,
    .id = id,
    .repeatInS = repeatInS,
    .action = actionname
  };
  cronjobs.emplace(owner, [&](auto& obj) { //initial payment by contract account
      obj.cronjob = cronjob;
    });
  auto tnow = now();
  eosio_assert(tnow < firstRunUTC, "First run must be in future");
  uint64_t timeTillRun = firstRunUTC - tnow;

  eosio::transaction txn{};
  txn.actions.emplace_back(
                           eosio::permission_level{owner, "active"_n},
                           _self,
                           "runandsched"_n,
                           std::make_tuple(owner, id, uint64_t(0)));
  txn.delay_sec = timeTillRun;

  uint128_t txid = (((uint128_t) id.value) << 32) + owner.value;
  cancel_deferred(txid);
  txn.send(txid, owner, false); //replace not allowed by EOS
  D(print("Job scheduled to start in ", timeTillRun, "s with txId ", txid, " \n"));
}

ACTION cronjob::runandsched(name owner, name id, uint64_t nRecure)
{
  require_auth(owner);
  // if(nRecure > 10) //for testing
  //   {
  //     D(print("Stopping at ", nRecure, "th time. \n"));
  //     return;
  //   }
  cronjobs_t cronjobs(_self, owner.value);
  D(print("Recurring number: ", nRecure, " \n"));
  auto jobIt = cronjobs.find(id.value);
  eosio_assert(jobIt != cronjobs.end(), "Cannot find job");
  auto job = jobIt->cronjob;

  //run
  action(permission_level{job.acc,"active"_n},
         job.acc,
         job.action,
         std::make_tuple()).send();
  updateLastRun(cronjobs, job.id);

  //schedule next (deffered)
  eosio::transaction txn{};
  txn.actions.emplace_back(
                           eosio::permission_level{owner, "active"_n},
                           _self,
                           "runandsched"_n,
                           std::make_tuple(owner, id, nRecure+1));
  txn.delay_sec = job.repeatInS;
  uint128_t txid = (((uint128_t) job.id.value) << 32) + owner.value;
  txn.send(txid, owner, false); //replace not allowed by EOS
}

void cronjob::updateLastRun(cronjobs_t& cronjobs, name id)
{
  auto jobIt = cronjobs.find(id.value);
  eosio_assert(jobIt != cronjobs.end(), "Cannot find job");
  cronjobs.modify(jobIt, _self, [&](auto& obj) {
      obj.lastRun = now();
    });
}

ACTION cronjob::rmalljobs(name owner)
{
  eosio_assert(has_auth(owner) or has_auth(_self), "auth from contract or caller needed");
  cronjobs_t cronjobs(_self, owner.value);
  unsigned int i = 0;
  bool left = false;
  for(auto it = cronjobs.begin(); it != cronjobs.end(); i++)
    {
      if(i > 100)
        {
          left = true;
          i -= 1;
          break;
        }

      uint128_t txid = (((uint128_t) it->cronjob.id.value) << 32) + it->cronjob.acc.value;
      cancel_deferred(txid);
      it = cronjobs.erase(it);
    }
  D(print("Deleted jobs: ", i, (left?" Jobs left":"")," \n"));
}

ACTION cronjob::rmjob(name owner, name id)
{
  eosio_assert(has_auth(owner) or has_auth(_self), "auth from contract or caller needed");
  cronjobs_t cronjobs(_self, owner.value);
  auto it = cronjobs.find(id.value);
  eosio_assert(it != cronjobs.end(), "Job already deleted?");
  uint128_t txid = (((uint128_t) it->cronjob.id.value) << 32) + it->cronjob.acc.value;
  cancel_deferred(txid);
  cronjobs.erase(it);
}

EOSIO_DISPATCH( cronjob, (insertjob)(runandsched)(rmalljobs)(rmjob))
