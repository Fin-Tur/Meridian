#pragma once
#include <string>

enum class currency{
    USD,
    EUR,
    GBP,
    JPY,
    CHF,
    CAD,
    AUD,
    NZD,
    CNY,
    SEK,
    NOK,
    DKK,
    ZAR,
    INR,
    BRL,
    MXN,

    UNKNOWN
};

currency convert_curr_fstr(std::string currency_str){
    if(currency_str == "USD") return currency::USD;
    else if(currency_str == "EUR") return currency::EUR;
    else if(currency_str == "GBP") return currency::GBP;
    else if(currency_str == "JPY") return currency::JPY;
    else if(currency_str == "CHF") return currency::CHF;
    else if(currency_str == "CAD") return currency::CAD;
    else if(currency_str == "AUD") return currency::AUD;
    else if(currency_str == "NZD") return currency::NZD;
    else if(currency_str == "CNY") return currency::CNY;
    else if(currency_str == "SEK") return currency::SEK;
    else if(currency_str == "NOK") return currency::NOK;
    else if(currency_str == "DKK") return currency::DKK;
    else if(currency_str == "ZAR") return currency::ZAR;
    else if(currency_str == "INR") return currency::INR;
    else if(currency_str == "BRL") return currency::BRL;
    else if(currency_str == "MXN") return currency::MXN;
    return currency::UNKNOWN;
    
}    
