// Copyright 2018 Your Name <your_email>

#ifndef INCLUDE_MAIN_HPP_
#define INCLUDE_MAIN_HPP_
#include <iostream>
#include <string>
#include <random>
#include <utility>
#include <thread>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/unordered_map.hpp>
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <boost/program_options.hpp>
#include <mutex>
#include <vector>
#include <list>
#include <picosha2.hpp>
#include <constants.hpp>

//Объявление всех методов используемых в лабе

//Rocksdb::ColumnFamilyHandle - имеется поддержка семейства столбцов.
//Каждая пара ключ-значение свяхана ровно с одним семейством столбцов
using FContainer = std::list<std::unique_ptr<rocksdb::ColumnFamilyHandle>>; //создание контейнера (или двусвязного списка)
using FDescriptorContainer = std::vector<rocksdb::ColumnFamilyDescriptor>;
using FHandlerContainer = std::list<std::unique_ptr<rocksdb::ColumnFamilyHandle>>; //создание контейнера (или двусвязного списка)
using StrContainer = boost::unordered_map<std::string, std::string>; //unordered_map — это связанный контейнер, в котором хранятся элементы, образованные комбинацией значения ключа и сопоставленного значения.
namespace po = boost::program_options;

class DBHashCreator {
public:
    //_ - приватное поле класса
    explicit DBHashCreator(std::string path) : _path(path) {} //если передать только путь, то вызывается данный конструктор

    DBHashCreator(std::string path, std::size_t threadCount, std::string logLVL) : _path(path), _logLVL(logLVL), _threadCountHash(threadCount){} //инициализация привытныъ полей класса

    void createDB();

    FContainer randomFillFamilies();

    void randomFillStrings(const FContainer &container);

    FDescriptorContainer getFamilyDescriptors();

    FHandlerContainer openDB(const FDescriptorContainer &);

    StrContainer getStrs(rocksdb::ColumnFamilyHandle *);

    void getHash(rocksdb::ColumnFamilyHandle *, StrContainer);

    std::string getRandomString(std::size_t);

    void randomFill();

    void startHash(FHandlerContainer *, std::list<StrContainer> *);

    void startThreads();

private:
    std::string _path;
    std::string _logLVL;
    std::unique_ptr<rocksdb::DB> _db; //умный указатель на балтика девять
    std::size_t _threadCountHash = DEFAULT_THREAD_HASH;
    std::mutex _mutex;
};
#endif // INCLUDE_MAIN_HPP_
