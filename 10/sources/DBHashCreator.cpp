// Copyright 2018 Your Name <your_email>
#include <main.hpp>
#include <constants.hpp>
#include <logs.hpp>
//Открытие базы данных
FHandlerContainer DBHashCreator::openDB (const FDescriptorContainer &descriptors) {
    FHandlerContainer handlers;//FHandlerContainer-контейнер
    std::vector < rocksdb::ColumnFamilyHandle * > newHandles;//создание вектора указателей newHandles типа ColumnFamilyHandle
    rocksdb::DB *dbStrPtr; //Создание указателя dbStrPtr на DB
//Значения этого типа возвращаются большинством функций, в которых может возникнуть ошибка. Перменная status приравнивается к возвращенному значению функцию
    rocksdb::Status status = rocksdb::DB::Open(rocksdb::DBOptions(), _path, descriptors, &newHandles, &dbStrPtr);//newHandles-указатели на семьи, dbStrPtr-указатель на саму  бд
    assert(status.ok()); //if 0 -> exit
// Функция assert оценивает выражение, которое передается ей в качестве аргумента,если аргумент-выражение равно нулю (т.е. выражение ложно),
// сообщение записывается на стандартное устройство вывода ошибок и  работа программы прекращается.
    _db.reset(dbStrPtr);//очищает указатель _db и записывает dbStrPtr в _db

    for (rocksdb::ColumnFamilyHandle *ptr : newHandles) {
        handlers.emplace_back(ptr); //emplace_back-создает объект в конце вектора без лишнего копирования //запись семей бд в контейнер handlers
    }

    return handlers;//возвращает бд
}

//в каждой бд есть семья и у каждой семьи есть хендлер(указатель на данную семью)





FDescriptorContainer DBHashCreator::getFamilyDescriptors() {
    rocksdb::Options options; //создаем объект класса options
    std::vector<std::string> family;
    FDescriptorContainer descriptors;//FDescriptorContainer-вектор
    rocksdb::Status status = rocksdb::DB::ListColumnFamilies(rocksdb::DBOptions(), _path,&family);  // Получение списка семейств столбцов и занесение их в family
    // при этом можем открыть только одно подмножество столбцов, так как одно семейство столбцов соответсвует одной паре ключ-значение
    assert(status.ok()); //if 0 -> exit
    // Функция assert оценивает выражение, которое передается ей в качестве аргумента,если аргумент-выражение равно нулю (т.е. выражение ложно),
    // сообщение записывается на стандартное устройство вывода ошибок и  работа программы прекращается.

    for (const std::string &familyName : family) //перебор элементов вектора family и заносим каждый элемент в descriptors
    {
        descriptors.emplace_back(familyName, rocksdb::ColumnFamilyOptions());//emplace_back-создает объект в конце вектора без лишнего копирования
        //rocksdb::ColumnFamilyOptions()-определяет параметры, относящиеся к одному семейству столбцов
    }
    return descriptors; //возвращает список семейств столбцов
}

StrContainer DBHashCreator::getStrs(rocksdb::ColumnFamilyHandle *family) {
    boost::unordered_map <std::string, std::string> dbCase;// создание контейнер(map) dbCase
    //занести все пары (ключ, значение) из базы данных в переменные
    std::unique_ptr <rocksdb::Iterator> it (_db->NewIterator(rocksdb::ReadOptions(), family)); //unique_ptr - умный указатель //чтение из базы данных
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        std::string value = it->value().ToString();
        dbCase[key] = value;
    }
    return dbCase; //возвращение контейнера dbCase с парами (ключ-значение) из бд
}

void DBHashCreator::getHash (rocksdb::ColumnFamilyHandle *family, StrContainer strContainer) {
    for (auto it = strContainer.begin(); it != strContainer.end(); ++it) {
        std::string hash = picosha2::hash256_hex_string(it->first + it->second);//hash256 - Алгоритм хеширования для подсчета конрольной суммы. it->first - key
        std::cout << "key: " << it->first << " hash: " << hash << std::endl;
        logs::logInfo(it->first, hash);// Логирование, с умным временем (время отсчета от начала программы)
        rocksdb::Status status = _db->Put(rocksdb::WriteOptions(),family, it->first, hash); //занесение в базу данных хеш_значения
        assert(status.ok()); // проверка наличия ошибок, обнаруженных во время сканирования

    }
}




void DBHashCreator::startHash (FHandlerContainer *handlers, std::list <StrContainer> *StrContainerList) {
    while (!handlers->empty()) {//если handlers не пустой
        _mutex.lock();//блокируем поток
        if (handlers->empty()) { //если handlers пустой
            _mutex.unlock(); //разблокируем поток
            continue; // переход в конец тела цикла
        }
        auto &family = handlers->front();//создаем переменную (ссылку) авто_типа. Front возвращение на первый элемент вектора
        handlers->pop_front(); //Удаляем первый элемент вектора

        StrContainer strContainer = StrContainerList->front(); // создали объект класса и передаем первый элемент
        StrContainerList->pop_front();  // удалили первый элемент
        _mutex.unlock();// разблокирование потока
        getHash(family.get(), strContainer); //family.get()-считывает символ и возвращает его значение ????
    }
}



void DBHashCreator::startThreads()
{
    auto deskriptors = getFamilyDescriptors(); // Автоматическое определение типа данных. //getFamilyDescriptors()-возвращает список семейств столбцов
    auto handlers = openDB(deskriptors); // Открытие БД //возвращает - элементы (семьи)бд

    std::list<StrContainer> StrContainerList; // List - контейнер или двусвязный список

    for (auto &family : handlers) // перебор кажного элемента handlers
    {
        StrContainerList.push_back(getStrs(family.get())); // добавление результата в конец StrContainerList результат getStrs. dbcase класса StrContainer.
    //family.get()-считывает символ и возвращает его значение()
    //getStrs - возвращение контейнера map dbCase с парами (ключ-значение) из бд  (преобразование в строку)
    }

    std::vector<std::thread> threads; //создание вектора потоков
    threads.reserve(_threadCountHash); // Выделяем память под количество потоков,которое задается _threadCountHash
    for (size_t i = 0; i < _threadCountHash; ++i)
    {
        threads.emplace_back(std::thread(&DBHashCreator::startHash,this,&handlers, &StrContainerList)); //вектор потоков //вызывается метод startHash и туда бередается &handlers, &StrContainerList
    }
    for (auto &th : threads)
    {
        th.join(); // запуск потоков
    }
}
