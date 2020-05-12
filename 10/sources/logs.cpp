// Copyright 2018 Your Name <your_email>

#include <main.hpp>
#include <logs.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;

void logs::logInFile()
{ // Создание файла, и его запись для уровня логирования info
    logging::add_file_log( // наследование от класса logging метода add_file_log
            keywords::file_name = "/log/info.log", // задаём имя и путь файла
            keywords::rotation_size = 256 * 1024 * 1024, // размер файла для логов
            keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0), // Точка поворота (Час,Мин,Сек), вывод времени с точки запуска программы(таймер)
            keywords::filter = logging::trivial::severity >= logging::trivial::info, //Глобальный фильтр в ядре библиотеки по уровню логирования
            keywords::format = // Формат вывода Лог.файла
                    (expr::stream // Вывод в поток
                            << boost::posix_time ::second_clock::local_time() // Время
                            << " : <" << logging ::trivial::severity // Уровень логирования
                            << "> " << expr::smessage)); // Сообщение

// Создание файла, и его запись для уровня логирования trace
    logging::add_file_log(
            keywords::file_name = "/log/trace.log", // См. уровни логирования (Trace and info)
            keywords::rotation_size = 256 * 1024 * 1024,
            keywords::time_based_rotation = sinks::file ::rotation_at_time_point(0, 0, 0),
            keywords::filter = logging::trivial::severity >= logging::trivial::trace,
            keywords::format =
                    (expr::stream
                            << boost::posix_time ::second_clock::local_time()
                            << " : <" << logging::trivial::severity
                            << "> " << expr::smessage));
}

void logs::logInfo(const std::string &key, const std::string &hash) // Метод вывода logInfo в сообщение smessage
{
    BOOST_LOG_TRIVIAL(info) << "Thread with ID: " << std::this_thread::get_id() //get_id()-вывод номера потока
                            << " Key: " << key << " Hash: " << hash << std::endl;
}


void logs::logTrace(const std::string &key, const std::string &value) // Метод вывода logTrace в сообщение smessage
{
    BOOST_LOG_TRIVIAL(trace) << "Thread with ID: " << std::this_thread::get_id()
                             << " Key: " << key << " Value: " << value << std::endl;
}