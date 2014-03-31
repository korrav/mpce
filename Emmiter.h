/*
 * Emmiter.h
 *
 *  Created on: 02 марта 2014 г.
 *      Author: andrej
 */

#ifndef EMMITER_H_
#define EMMITER_H_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <string>

using std::string;
namespace mpce_n {

/*
 *
 */
class Emmiter {
	static int sock_; //сокет для передачи данных в СКАЗ
	static sockaddr_in addrIn_; //адрес интерфейса СКАЗ
	enum id_command {
		ENABLE_DEBUG,
		GET_ENABLE_DEBUG,
		DISABLE_DEBUG,
		STOP_MEAS,
		START_MEAS,
		SET_TIMINGS,
		DEBUG_INFO,
		GET_TIMINGS,
	}; //идентификаторы команд
	enum status_answer {
		NOT_OK, OK
	}; //статус ответа на команду

	struct timings_meas { //структура пакета изменения характеристик временных параметров измерения
		int cur_tarTime_sec;
		int cur_tarTime_nsec;
		int restart;
		int tarTime_sec;
		int tarTime_nsec;
		int subtOnAmp;
		int w_subtOnAmp;
		int addtOfHyd;
		int w_addtOfHyd;
		int addtADC1;
	};

	struct head_data_adc { //шапка пакета данных
		int sec;
		int nsec;
		int hash;
	};

	string name_; //имя эммитера
	static head_data_adc cur_head; //шапка последнего принятого пакета данных
	static string root_dir_; //корневая папка
	static string month_; //название текущего месяца
	static string cur_dir_; //папка, куда был записан последний принятый пакет данных
	static int id_month_; //числовой идентификатор текущего месяца
	static void trans_command(void* pbuf, const size_t& size, const int& id); //передача команды
	static void init_timings_meas(timings_meas& t); //инициализация структуры  timings_meas
public:
	static const int LAST = -1; //последний пакет в последовательности
	enum typ {
		DATA, COMMAND, ANSWER
	}; //идентификаторы типа буфера

	int id_; //идентификатор модуля
	static int allId_; //идентификатор посылки, предназначенной для всех эммитеров системы
	struct head { //структура, в которую оборачиваются команды и ответы
		int size; //размер буфера
		int count; //порядковый номер буфера
		int dst;	//пункт назначения
		int src;	//источник пакета
		int type;	//тип буфера
		bool operator ==(const head& h) {
			if (size == h.size && count == h.count && dst == h.dst
					&& src == h.src && type == h.type)
				return true;
			else
				return false;
		}
		bool operator !=(const head& h) {
			return !(*this == h);
		}
	};
	enum srcdst {
		BAG = 1, INTERFACE = 2, MASTER = 4, SLAVE = 8
	}; //идентификаторы узлов системы

	struct d_info { // структура, содержащая значения регистров фитера
		int id;
		int ptp_ctl;
		int ptp_sts;
		int ptp_tsts;
		int ptp_rate;
		int ptp_trig0;
		unsigned int ptp_trig0_sec;
		unsigned int ptp_trig0_nsec;
		unsigned int ptp_trig0_puls;
		unsigned int ptp_trig2;
		unsigned int ptp_trig2_sec;
		unsigned int ptp_trig2_nsec;
		unsigned int ptp_trig2_puls;
		unsigned int ptp_trig6;
		unsigned int ptp_trig6_sec;
		unsigned int ptp_trig6_nsec;
		unsigned int ptp_trig6_puls;
		unsigned int ptp_trig7;
		unsigned int ptp_trig7_sec;
		unsigned int ptp_trig7_nsec;
		unsigned int ptp_trig7_puls;
		unsigned int ptp_txcfg0;
		unsigned int ptp_txcfg1;
		unsigned int ptp_rxcfg0;
		unsigned int ptp_rxcfg1;
		unsigned int ptp_rxcfg3;
		unsigned int ptp_rxcfg4;
		unsigned int ptp_coc;
		unsigned int ptp_clksrc;
		unsigned int ptp_gpiomon;
	};
	static void set_ethernet_configuration(const int& s,
			const sockaddr_in& addr);

	static void enable_debug(const int& num, const unsigned& time,
			const int& id = allId_); /*позволение передачи в центр отладочной информации
			 (num - количество передаваемых пакетов c отладочной информацией, если num = -1 - без ограничений; time - период в миллисекундах*/
	static void get_enable_debug(const int& id = allId_); //запросить параметры выдачи отладочной информации в центр
	static void disable_debug(const int& id = allId_); //запретить выдачу отладочной информации в центр
	static void stop_measurement(const int& id = allId_); //остановить процесс измерения
	static void start_measurement(const int& id = allId_); //запустить процесс измерения
	static void set_sync(const int& id, int restart = -1); //установка системного времени в модули СКАЗ
	static void set_tar(const int& id, const int& sec, const int& nsec,
			int restart = -1); //установка периода повторения измерений
	static void set_addtADC1(const int& id, const int& nsec, int restart = -1); //установка времени между началом измерения и стартом ацп п/я
	static void set_addtOfHyd(const int& id, const int& nsec, int restart = -1); /*установка времени между началом измерения и шунтирования
	 гидрофона*/
	static void set_subtOnAmp(const int& id, const int& nsec, int restart = -1); /*установка времени между началом измерения и включением
	 усилителя*/
	static void set_w_addtOfHyd(const int& id, const int& nsec,
			int restart = -1); /*установка временного промежутка шунтирования гидрофона*/
	static void set_w_subtOnAmp(const int& id, const int& nsec,
			int restart = -1); /*установка временного промежутка включения усилителя*/
	void receive_ans(const void* pbuf, const size_t& size); //обработка ответа на команды
	void receive_data(const void* pbuf, const size_t& size); //обработка данных
	static void set_root_dir(const string &name); //установить текщую корневую папку

	Emmiter(const int& id, const string& n);
	virtual ~Emmiter();
};

} /* namespace mpce_n */

#endif /* EMMITER_H_ */
