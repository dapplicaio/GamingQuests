#include "game.hpp"

void game::add_voting_with_options(const name &voting_name, const std::vector<std::string> &options)
{
    votings_t votings(get_self(), voting_name.value);

    for (uint64_t index = 0; index < options.size(); index++)
    {
        votings.emplace(get_self(), [&](auto &new_row)
                        {
            new_row.id = index;
            new_row.voting_option = options[index];
            new_row.voted = {};
            new_row.total_voted_option = 0; });
    }
}

std::string game::get_voting_winner_clear(const name &voting_name)
{
    std::string winner = "";
    uint64_t max_voted = 0;
    uint64_t id = 0;

    votings_t votings(get_self(), voting_name.value);

    balance_t balance_table(get_self(), get_self().value);

    while (true)
    {
        auto option_iter = votings.find(id);

        if (option_iter == votings.end())
        {
            break;
        }

        if (option_iter->total_voted_option > max_voted)
        {
            max_voted = option_iter->total_voted_option;
            winner = option_iter->voting_option;
        }

        for (const auto& player_amount : option_iter->voted)
        {
            auto player_balance_iter = balance_table.require_find(player_amount.first.value, "Player was deleted");
            balance_table.modify(player_balance_iter, get_self(), [&](auto& row)
            {
                row.quantity.amount += player_amount.second;
            });
        }

        votings.erase(option_iter);

        id++;
    }

    return winner;
}

std::string game::clear_voting_from_vinfo(const name &voting_name)
{
    vinfo_t vinfo(get_self(), get_self().value);
    auto voting_info_iter = vinfo.require_find(voting_name.value, "No such voting");

    std::string variable_name = voting_info_iter->variable_to_change;
    uint64_t min_staked = voting_info_iter->min_staked_tokens;
    uint64_t total_staked = voting_info_iter->total_staked;

    check(total_staked >= min_staked, "Minimal rate is not reached yet");

    if (voting_info_iter->time_to_vote != 0)
    {
        check((current_time_point().sec_since_epoch() - voting_info_iter->creation_time) >= voting_info_iter->time_to_vote,
              "Time for voting is not over yet");
    }

    if (voting_info_iter->max_staked_tokens != 0)
    {
        check(total_staked >= voting_info_iter->max_staked_tokens, "Voting limit is not reached yet");
    }

    vinfo.erase(voting_info_iter);

    return variable_name;
}

void game::close_general_voting(const name &voting_name)
{
    clear_voting_from_vinfo(voting_name);
    std::string winner = get_voting_winner_clear(voting_name);

    // add to closed
    closed_votings_t closed_votings(get_self(), get_self().value);
    uint64_t voting_id = closed_votings.available_primary_key();

    closed_votings.emplace(get_self(), [&](auto &new_row)
                           {
        new_row.voting_id = voting_id;
        new_row.voting_name = voting_name;
        new_row.winner_option = winner; });
}

void game::close_automatic_voting(const name &voting_name)
{
    std::string variable_name = clear_voting_from_vinfo(voting_name);
    std::string winner = get_voting_winner_clear(voting_name);

    // do transaction
    mconfigs_t config_table(get_self(), get_self().value);
    auto config_iter = config_table.require_find(stringToUint64(variable_name), "Variable in question was deleted");
    config_table.modify(config_iter, get_self(), [&](auto &row)
                        { row.variable_value = winner; });
}

void game::crgenvt(
    const name &player,
    const name &voting_name,
    const std::vector<std::string> &options,
    uint64_t min_staked_tokens,
    uint32_t time_to_vote,
    uint64_t max_staked_tokens)

{
    require_auth(player);
    vinfo_t vinfo(get_self(), get_self().value);
    check(vinfo.find(voting_name.value) == std::end(vinfo), "Voting with this name is active");

    // add to info

    vinfo.emplace(get_self(), [&](auto &new_row)
                  {
        new_row.voting_name = voting_name;
        new_row.creator = player;
        new_row.min_staked_tokens = min_staked_tokens;
        new_row.max_staked_tokens = max_staked_tokens;
        new_row.total_staked = 0;
        new_row.creation_time = current_time_point().sec_since_epoch();
        new_row.time_to_vote = time_to_vote; });

    add_voting_with_options(voting_name, options);
}

void game::cravote(
    const name &player,
    const name &voting_name,
    const std::string &variable_name,
    const std::vector<CONFIG_TYPE> &options_typed,
    int64_t min_staked_tokens,
    int32_t time_to_vote,
    uint64_t max_staked_tokens)
{
    require_auth(player);
    vinfo_t vinfo(get_self(), get_self().value);
    check(vinfo.find(voting_name.value) == std::end(vinfo), "Voting with this name is active");

    // check type correctness

    std::vector<std::string> options;
    mconfigs_t configs_table(get_self(), get_self().value);
    auto config_iter = configs_table.require_find(stringToUint64(variable_name), "No such variable in configs table");
    std::string variable_type = config_iter->variable_type;

    for (const auto &option : options_typed)
    {
        auto variant_data = get_config_variant_data(option);
        check(variant_data.first == variable_type, "Types mismatch in options");
        options.push_back(variant_data.second);
    }

    // add to info

    vinfo.emplace(get_self(), [&](auto &new_row)
                  {
        new_row.voting_name = voting_name;
        new_row.creator = player;
        new_row.min_staked_tokens = min_staked_tokens;
        new_row.max_staked_tokens = max_staked_tokens;
        new_row.total_staked = 0;
        new_row.creation_time = current_time_point().sec_since_epoch();
        new_row.time_to_vote = time_to_vote;
        new_row.variable_to_change = variable_name; });

    add_voting_with_options(voting_name, options);
}

void game::gvote(
    const name &player,
    const name &voting_name,
    const std::string &voting_option,
    uint64_t voting_power)
{
    require_auth(player);
    balance_t balance_table(get_self(), get_self().value);
    auto player_balance_iter = balance_table.require_find(player.value, "No tokens staked");
    check(player_balance_iter->quantity.amount >= voting_power, "Not enough tokens to vote with that voting power");

    vinfo_t vinfo(get_self(), get_self().value);
    auto voting_info_iter = vinfo.require_find(voting_name.value, "No such voting");

    // check time

    if (voting_info_iter->time_to_vote != 0)
    {
        check((current_time_point().sec_since_epoch() - voting_info_iter->creation_time) <= voting_info_iter->time_to_vote,
              "Time for voting is over");
    }

    // check max limit

    if (voting_info_iter->max_staked_tokens != 0)
    {
        check(voting_info_iter->total_staked < voting_info_iter->max_staked_tokens, "Max limit for staking is reached");
    }

    votings_t votings(get_self(), voting_name.value);
    auto option_iter = votings.end();
    auto current_iter = votings.end();
    uint64_t id = 0;

    while (true)
    {
        current_iter = votings.find(id);

        if (current_iter == votings.end())
        {
            break;
        }

        if (current_iter->voting_option == voting_option)
        {
            option_iter = current_iter;
        }

        check(current_iter->voted.find(player) == current_iter->voted.end(), "Already voted");

        id++;
    }

    check(option_iter != votings.end(), "No such option");

    votings.modify(option_iter, get_self(), [&](auto &row)
                   {

        row.voted[player] = voting_power;
        row.total_voted_option += voting_power; });

    vinfo.modify(voting_info_iter, get_self(), [&](auto &row)
                 { row.total_staked += voting_power; });

    balance_table.modify(player_balance_iter, get_self(), [&](auto& row)
    {
        row.quantity.amount -= voting_power;
    });

    // check max limit and close if it's achieved
    if (voting_info_iter->max_staked_tokens != 0)
    {
        if (voting_info_iter->total_staked >= voting_info_iter->max_staked_tokens)
        {
            if (voting_info_iter->variable_to_change.empty())
            {
                close_general_voting(voting_name);
            }
            else
            {
                close_automatic_voting(voting_name);
            }
        }
    }
}

void game::clsvt(const name &voting_name)
{
    vinfo_t vinfo(get_self(), get_self().value);
    auto voting_info_iter = vinfo.require_find(voting_name.value, "No such voting");

    require_auth(voting_info_iter->creator);

    if (voting_info_iter->variable_to_change.empty())
    {
        close_general_voting(voting_name);
    }
    else
    {
        close_automatic_voting(voting_name);
    }
}

void game::cancelvote(const name& player, const name& voting_name)
{
    require_auth(player);
    
    vinfo_t vinfo(get_self(), get_self().value);
    auto voting_info_iter = vinfo.require_find(voting_name.value, "No such voting");

    balance_t balance_table(get_self(), get_self().value);
    auto player_balance_iter = balance_table.require_find(player.value, "No staked tokens for this player");
    
    votings_t votings(get_self(), voting_name.value);
    auto option_iter = votings.end();
    auto current_iter = votings.end();

    uint64_t voting_power = 0;
    uint32_t id = 0;

    while (true)
    {
        current_iter = votings.find(id);

        if (current_iter == votings.end())
        {
            break;
        }


        auto player_voting_iter = current_iter->voted.find(player);
        if (player_voting_iter != current_iter->voted.end())
        {
            voting_power = player_voting_iter->second;

            votings.modify(current_iter, get_self(), [&](auto& row)
            {
                row.voted.erase(player_voting_iter);
                row.total_voted_option -=  voting_power;
            });

            break;
        }

        id++;
    }

    if (voting_power != 0)
    {
        vinfo.modify(voting_info_iter, get_self(), [&](auto& row)
        {
            row.total_staked -= voting_power;
        });

        balance_table.modify(player_balance_iter, get_self(), [&](auto& row)
        {
            row.quantity.amount += voting_power;
        });
    }
}