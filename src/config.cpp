#include "game.hpp"

std::pair<std::string, std::string> game::get_config_variant_data(const CONFIG_TYPE &var)
{
    if (std::holds_alternative<std::string>(var))
    {
        return {"string", std::get<std::string>(var)};
    }
    else if (std::holds_alternative<float>(var))
    {
        return {"float", std::to_string(std::get<float>(var))};
    }
    else if (std::holds_alternative<uint32_t>(var))
    {
        return {"uint32", std::to_string(std::get<uint32_t>(var))};
    }
    else if (std::holds_alternative<int32_t>(var))
    {
        return {"int32", std::to_string(std::get<int32_t>(var))};
    }
    else
    {
        return {"error", ""};
    }
}

void game::setcnfg(
    const std::string &variable_name,
    const CONFIG_TYPE &variable)
{
    require_auth(get_self());
    mconfigs_t configs_table(get_self(), get_self().value);

    auto var_iter = configs_table.find(stringToUint64(variable_name));

    auto variant_data = get_config_variant_data(variable);

    if (var_iter == configs_table.end())
    {
        configs_table.emplace(get_self(), [&](auto &new_row)
                              {
        new_row.variable_name = variable_name;
        new_row.variable_type = variant_data.first;
        new_row.variable_value = variant_data.second; });
    }
    else
    {
        check(var_iter->variable_type == variant_data.first, "Types mismatch");
        configs_table.modify(var_iter, get_self(), [&](auto &row)
                             { row.variable_value = variant_data.second; });
    }
}
