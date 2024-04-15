#ifndef GAME_H
#define GAME_H

#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>
#include "atomicassets.hpp"

using namespace eosio;

class [[eosio::contract]] game : public contract
{
public:
  using contract::contract;

  typedef std::variant<float, uint32_t, int32_t, std::string> CONFIG_TYPE;

  // listening atomicassets transfer
  [[eosio::on_notify("atomicassets::transfer")]] void receive_asset_transfer(
      const name &from,
      const name &to,
      std::vector<uint64_t> &asset_ids,
      const std::string &memo);

  [[eosio::on_notify("eosio.token::transfer")]] // tokencont change for your token contract
  void
  receive_token_transfer(
      const name &from,
      const name &to,
      const asset &quantity,
      const std::string &memo);

  [[eosio::action]] void claim(const name &owner, const uint64_t &farmingitem);

  [[eosio::action]] void upgradeitem(
      const name &owner,
      const uint64_t &item_to_upgrade,
      const uint8_t &next_level,
      const uint64_t &staked_at_farmingitem);

  [[eosio::action]] void upgfarmitem(const name &owner, const uint64_t &farmingitem_to_upgrade, const bool &staked);

  [[eosio::action]] void addblend(
      const std::vector<int32_t> blend_components,
      const int32_t resulting_item);

  [[eosio::action]] void setratio(const std::string &resource, const float &ratio);

  [[eosio::action]] void swap(const name &owner, const std::string &resource, const float &amount2swap);

  [[eosio::action]] void createvoting(
      const name &player,
      const std::string &resource_name,
      const float &new_ratio);

  [[eosio::action]] void vote(
      const name &player,
      const uint64_t &voting_id);

  [[eosio::action]] void crgenvt(
      const name &player,
      const name &voting_name,
      const std::vector<std::string> &options,
      uint64_t min_staked_tokens,
      uint32_t time_to_vote,
      uint64_t max_staked_tokens);

  [[eosio::action]] void gvote(
      const name &player,
      const name &voting_name,
      const std::string &voting_option,
      uint64_t voting_power);

  [[eosio::action]] void cancelvote(const name &player, const name &voting_name);

  [[eosio::action]] void cravote(
      const name &player,
      const name &voting_name,
      const std::string &variable_name,
      const std::vector<CONFIG_TYPE> &options_typed,
      int64_t min_staked_tokens,
      int32_t time_to_vote,
      uint64_t max_staked_tokens);

  [[eosio::action]] void clsvt(
      const name &voting_name);

  [[eosio::action]] void setcnfg(
      const std::string &variable_name,
      const CONFIG_TYPE &variable);

  [[eosio::action]] void addquest(
    const name& player,
    const std::string& type,
    float required_amount
  );

  [[eosio::action]] void cmpltquest(
    const name& player,
    uint32_t quest_index
  );

private:
  // scope: owner

  struct quest
  {
    std::string type;
    float required_amount;
    float current_amount;
  };

  struct [[eosio::table]] quests_j 
  {
    name player;
    std::vector<quest> quests;

    uint64_t primary_key() const {return player.value;};
  };
  typedef multi_index<"quests"_n, quests_j> quests_t;

  struct [[eosio::table]] mconfig_j
  {
    std::string variable_name;
    std::string variable_type;
    std::string variable_value;

    uint64_t primary_key() const { return stringToUint64(variable_name); }
  };
  typedef multi_index<"config"_n, mconfig_j> mconfigs_t;

  struct [[eosio::table]] closed_votings_j
  {
    uint64_t voting_id;
    name voting_name;
    std::string winner_option;

    uint64_t primary_key() const { return voting_id; };
  };
  typedef multi_index<"closedvts"_n, closed_votings_j> closed_votings_t;

  struct [[eosio::table]] votings_info_j
  {
    name voting_name;
    name creator;
    uint64_t min_staked_tokens;
    uint64_t max_staked_tokens;
    uint64_t total_staked;
    uint32_t creation_time;
    uint32_t time_to_vote;
    std::string variable_to_change;

    uint64_t primary_key() const { return voting_name.value; };
  };
  typedef multi_index<"vtsinfo"_n, votings_info_j> vinfo_t;

  struct [[eosio::table]] voting_j
  {
    uint64_t id;
    std::string voting_option;
    std::map<name, uint64_t> voted;
    uint64_t total_voted_option;

    uint64_t primary_key() const { return id; }
  };
  typedef multi_index<"genvtngs"_n, voting_j> votings_t;

  struct [[eosio::table]] lboard_j
  {
    name account;
    uint64_t points;

    uint64_t primary_key() const { return account.value; };
  };
  typedef multi_index<"lboards"_n, lboard_j> lboards_t;

  struct [[eosio::table]] staked_j
  {
    uint64_t asset_id;                  // item
    std::vector<uint64_t> staked_items; // farming items

    uint64_t primary_key() const { return asset_id; }
  };
  typedef multi_index<"staked"_n, staked_j> staked_t;

  // scope: owner
  struct [[eosio::table]] resources_j
  {
    uint64_t key_id;
    float amount;
    std::string resource_name;

    uint64_t primary_key() const { return key_id; }
  };
  typedef multi_index<"resources"_n, resources_j> resources_t;

  // scope:contract
  struct [[eosio::table]] blends_j
  {
    uint64_t blend_id;
    std::vector<int32_t> blend_components;
    int32_t resulting_item;

    uint64_t primary_key() const { return blend_id; }
  };
  typedef multi_index<"blends"_n, blends_j> blends_t;

  struct [[eosio::table]] balance_j
  {
    name owner;
    asset quantity;

    uint64_t primary_key() const { return owner.value; }
  };
  typedef multi_index<"balance"_n, balance_j> balance_t;

  struct [[eosio::table]] resourcecost_j
  {
    uint64_t key_id;
    std::string resource_name;
    float ratio; // if user swap 100 wood and ration is 25 it means that user will receive 4 tokens

    uint64_t primary_key() const { return key_id; }
  };
  typedef multi_index<"resourcecost"_n, resourcecost_j> resourcecost_t;

  struct [[eosio::table]] changeration_j
  {
    uint64_t voting_id;
    std::string resource_name;
    float new_ratio;
    std::map<name, asset> voted; // first is player name, second is voting power (in tokens)

    uint64_t primary_key() const { return voting_id; }
  };
  typedef multi_index<"changeration"_n, changeration_j> changeration_t;

  struct [[eosio::table]] avatars_j
  {
    name owner;
    std::vector<uint64_t> equipment;

    uint64_t primary_key() const { return owner.value; }
  };
  typedef multi_index<"avatarsc"_n, avatars_j> avatars_t;

  struct [[eosio::table]] stats_j
  {
    name owner;
    std::map<std::string, uint32_t> stats;

    uint64_t primary_key() const { return owner.value; }
  };
  typedef multi_index<"stats"_n, stats_j> stats_t;

  static const uint64_t stringToUint64(const std::string &str);

  void stake_farmingitem(const name &owner, const uint64_t &asset_id);
  void stake_items(const name &owner, const uint64_t &farmingitem, const std::vector<uint64_t> &items_to_stake);

  void increase_owner_resources_balance(const name &owner, const std::map<std::string, float> &resources);
  void reduce_owner_resources_balance(const name &owner, const std::map<std::string, float> &resources);

  void increase_tokens_balance(const name &owner, const asset &quantity);

  const std::pair<std::string, float> claim_item(atomicassets::assets_t::const_iterator &assets_itr, const uint8_t &upgrade_percentage, const uint32_t &time_now, const std::map<std::string, uint32_t> &stats);

  void upgrade_item(
      atomicassets::assets_t::const_iterator &assets_itr,
      const uint8_t &upgrade_percentage,
      const name &owner,
      const uint8_t &new_level,
      const uint32_t &time_now,
      const std::map<std::string, uint32_t> &stats);

  void upgrade_farmingitem(atomicassets::assets_t::const_iterator &assets_itr, const name &owner);

  void blend(const name &owner, const std::vector<uint64_t> asset_ids, const uint64_t &blend_id);

  void set_avatar(const name &owner, const uint64_t &asset_id);
  void set_equipment_list(const name &owner, const std::vector<uint64_t> &asset_ids);
  void set_equipment_item(const name &owner, const uint64_t asset_id, std::vector<uint64_t> &assets_to_return, std::map<std::string, uint32_t> &equiped_types);
  std::map<std::string, uint32_t> get_stats(const name &owner);
  void recalculate_stats(const name &owner);

  void incr_lb_points(const name &lbname, const name &account, uint64_t points);
  void decr_lb_points(const name &lbname, const name &account, uint64_t points);
  void set_lb_points(const name &lbname, const name &account, uint64_t points);

  float get_mining_power(const uint64_t asset_id, const std::map<std::string, uint32_t> &stats);
  void update_mining_power_lb(const name &account);

  std::pair<std::string, std::string> get_config_variant_data(const CONFIG_TYPE &var);

  void add_voting_with_options(const name &voting_name, const std::vector<std::string> &options);

  void close_general_voting(const name &voting_name);
  void close_automatic_voting(const name &voting_name);
  std::string get_voting_winner_clear(const name &voting_name);
  std::string clear_voting_from_vinfo(const name &voting_name);

  void update_quests(const name& player, const std::string& type, float update_amount, bool set = false);

  void tokens_transfer(const name &to, const asset &quantity);

  const int32_t get_upgrading_time(const uint8_t &end_level);

  // get mutable data from NFT
  atomicassets::ATTRIBUTE_MAP get_mdata(atomicassets::assets_t::const_iterator &assets_itr);
  // get immutable data from template of NFT
  atomicassets::ATTRIBUTE_MAP get_template_idata(const int32_t &template_id, const name &collection_name);
  // update mutable data of NFT
  void update_mdata(atomicassets::assets_t::const_iterator &assets_itr, const atomicassets::ATTRIBUTE_MAP &new_mdata, const name &owner);
};

#endif