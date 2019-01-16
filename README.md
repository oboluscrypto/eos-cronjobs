# Cronjob serive on EOS blockchain
**Simple:** deploy the contract, register your ACTIONS of your other contracts with times when they should be executed and they will be repeatably called automatically. Just like a cronjob...  

**How it works**: recursive deferred transactions.

**Note:** The tutorial and code is written for eosio-cpp version 1.4.1. Make sure you have the right version when compiling. You will also require cleos on a Linux environment.

## Deployment
This requires some basic knowledge about setting up accounts and deploying a contract. See a tutorial here for example:  
https://hackernoon.com/how-to-create-and-deploy-your-own-eos-token-1f4c9cc0eca1 or https://infinitexlabs.com/eos-development-tutorial-part-1/

In the following we use 2 accounts. *jobservice* which is where the cronjob service is deployed and *mycontract* which is where the contract with the actions sits.
First we need to build the contract with EOS CDT.  
```eosio-cpp -abigen cronjob.cpp -o cronjob.wasm && cleos set contract colin.job ../cronjob/```

Next we need to give permissions to the *jobservice* to run functions on *mycontract*.  
```cleos set account permission mycontract active '{"threshold": 1,"keys": [{"key": "EOS5bUj...YOUR.PUBLIC.KEY.HERE...AQq","weight": 1}], "accounts": [{"permission":{"actor":"jobservice","permission":"eosio.code"},"weight":1}]}' owner -p mycontract@owner```


## Registering your action
Now it's time to think about when the functions should run. We define an ID that identifies the job *testjob1* and let this job call the action *testprint*. The last arguments defines that it should repeat the action every 60 seconds with the first time running on UTC 2018-08-15 10:01.

```
export sched=`date --date "2018-08-15 10:01" +%s`;
export arg="[mycontract, testjob1, testprint, ${sched}, 60]";
echo $arg;
cleos push action jobservice insertjob "${arg}" -p mycontract@active
```

## Cancelling or updating a job
This requires permissions of *mycontract* or *jobservice*. For changing, remove the old job which also kill the current pending deferred tx. Then reinsert you modified job with the same id. You can only have one job per (owner,id).
```
cleos push action jobservice rmalljobs '["mycontract"]' -p mycontract@active
```
With rmjob you can also delete a specific job.


## Debug switch.
In the .hpp file, please switch the flag for DEBUG if you want to do your own development within the license requirements on this.

## Limitation of the EOS cronjob service
### Number of recursive calls
Tested with multiple thousand recursions but there might be a limit to the recursions. Be careful.

### ACTION arguments
It is difficult to store arguments of your ACTION in a table due to unknown type and number. There is a way to do it though, I leave this as an exercise for the user. It means that now you can call ACTIONS that do not require an argument. Just make a wrapper around that function and store the arguments yourself. Then it's also easy to change.

### Monthly or longer timeframes
Currently you can register a starttime and repeat time [s]. You cannot schedule the 1 day of the month or year. Also there might be an upper limit on deffered transactions. Be careful.

### Feedback on assertion error, timeout, resource limits, or other errors when running the actions
Feedback is tricky. As mentioned in this [issue](https://eosio.stackexchange.com/questions/3291/how-to-find-out-if-deferred-transaction-was-executed). In the jobservice you can check against the last executed flag and periodically make sure that everything is ok. The *mycontract* needs to provide resources for the called ACTIONS.
```cleos get table jobservice mycontract cronjobs```
