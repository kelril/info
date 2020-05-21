#pragma once
#include <async++.h>
#include <boost/process.hpp>
#include <boost/process/extend.hpp>
#include <boost/program_options.hpp>
#include <vector>
#include <iostream>
#include <signal.h>//сигналы используют  для информирования запущенных процессов о некоторых вознкиающих ошибках
#include <string>
#include <thread>
#include <chrono>//библиотека для работы со временем

using namespace boost::asio;
using namespace boost::process;
using namespace boost::process::extend;
using namespace boost::program_options;

//Объявление всех методов используемых в лабе
void build(int argc, char* argv[]);

void create_child(const std::string& command, const time_t& period);

void create_child(const std::string& command, const time_t& period, int& res);

void check_time(child& process, const time_t& period);

time_t time_now(); //time_t – арифметический тип, представляет собой целочисленное значение – число секунд, прошедших с 00:00, 1 января 1970
