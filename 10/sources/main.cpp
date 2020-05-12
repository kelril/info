// Copyright 2018 Your Name <your_email>
#include <main.hpp>
#include <constants.hpp>
#include <logs.hpp>

int main(int argc, char **argv) { //argc-кол-во параметров, **argv-строка
    po::options_description desc("short description");
    desc.add_options() //метод этого класса возвращает специальный прокси-объект, который определяет operator()
    //Параметры-это имя опции, информация о значении и опция Описание.
            ("help,h", "0 помощи") // вывод информации об аргументах
            ("log_level", po::value<std::string>(),"level logging")
            ("thread_count", po::value<unsigned>(),"count of threads")
            ("output", po::value<std::string>(),"path result");

    po::variables_map vm; //Этот экземпляр класса предназначен для хранения значений параметров и может хранить значения произвольных типов
    try {
        //далее вызов store, parse_command_line and notify заставляют vm содержать все параметры, найденные в командной строке.
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm); // Ловушка (уведомление), выводит компактные уведомления
    }

    catch (po::error &e) { // Обработка ошибки
        std::cout << e.what() << std::endl; // e.what - расшифровка ошибки
        std::cout << desc << std::endl;
        return 1;
    }
    if (vm.count("help")) {//vm.count не равно 0 //count()-возвращает число элементов
        std::cout << desc << std::endl; //вызов --help (справочной информации)
        return 1;
    }
    if (!vm.count("log_level") || !vm.count("thread_count") || !vm.count("output")) { // Проверка на структурированность и корректность ввода
        std::cout << "error: bad format" << std::endl << desc << std::endl;
        return 1;
    }

//Класс variables_map можно использовать точно так же, как std:: map, за исключением того, что хранящиеся там значения должны быть получены с помощью метода as
//ключ.as значение
    std::string logLVL = vm["log_level"].as<std::string>(); //в logLVL занесем значение соответствующее ключу "log_level"
    std::size_t threadCount = vm["thread_count"].as<unsigned>();
    std::string pathToFile = vm["output"].as<std::string>();

    logs::logInFile();//логирование в файл logs.cpp
    DBHashCreator db(pathToFile, threadCount, logLVL);//создание объекта класса DBHashCreator с параметрами pathToFile, threadCount, logLVL
    db.startThreads(); // Натравление потоков на БД
//    db.createDB();
//    db.randomFill();
    return 0;
}
