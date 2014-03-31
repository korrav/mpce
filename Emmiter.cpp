/*
 * Emmiter.cpp
 *
 *  Created on: 02 марта 2014 г.
 *      Author: andrej
 */

#include "Emmiter.h"
#include "string.h"
#include <string>
#include <time.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

using std::cout;
using std::endl;

namespace mpce_n {

sockaddr_in Emmiter::addrIn_;
int Emmiter::sock_;
int Emmiter::allId_ = 0;
Emmiter::head_data_adc Emmiter::cur_head;
string Emmiter::root_dir_ = ".";
string Emmiter::month_;
string Emmiter::cur_dir_;
int Emmiter::id_month_ = -1;

Emmiter::Emmiter(const int& id, const string& n) :
		name_(n), id_(id) {
	allId_ += id;
	return;
}

void Emmiter::set_ethernet_configuration(const int& s,
		const sockaddr_in& addr) {
	addrIn_ = addr;
	sock_ = s;
	return;
}

void Emmiter::trans_command(void* pbuf, const size_t& size, const int& id) {
	char* pack = new char[sizeof(struct head) + size];
	memcpy(pack + sizeof(struct head), pbuf, size);
	struct head* head = reinterpret_cast<struct head*>(pack);
	head->count = LAST;
	head->dst = id;
	head->size = size;
	head->src = BAG;
	head->type = COMMAND;
	sendto(sock_, pack, sizeof(struct head) + size, 0,
			reinterpret_cast<sockaddr*>(&addrIn_), sizeof(addrIn_));
	delete[] pack;
	return;

}

void Emmiter::enable_debug(const int& num, const unsigned & time,
		const int& id) {
	int buf[3] = { ENABLE_DEBUG, num, static_cast<int>(time) };
	trans_command(buf, sizeof(buf), id);
	return;
}

void Emmiter::get_enable_debug(const int& id) {
	int buf = GET_ENABLE_DEBUG;
	trans_command(&buf, sizeof(buf), id);
	return;
}

void Emmiter::disable_debug(const int& id) {
	int buf = DISABLE_DEBUG;
	trans_command(&buf, sizeof(buf), id);
	return;
}

void Emmiter::stop_measurement(const int& id) {
	int buf = STOP_MEAS;
	trans_command(&buf, sizeof(buf), id);
	return;
}

void Emmiter::start_measurement(const int& id) {
	int buf[3] = { START_MEAS, static_cast<int>(time(NULL)), 0 };
	trans_command(buf, sizeof(buf), id);
	return;
}

void Emmiter::init_timings_meas(timings_meas& t) {
	t.cur_tarTime_sec = -1;
	t.cur_tarTime_nsec = 0;
	t.addtADC1 = -1;
	t.addtOfHyd = -1;
	t.restart = -1;
	t.subtOnAmp = -1;
	t.tarTime_sec = -1;
	t.tarTime_nsec = 0;
	t.w_addtOfHyd = -1;
	t.w_subtOnAmp = -1;
	return;

}

void Emmiter::set_sync(const int& id, int restart) {
	timings_meas t;
	init_timings_meas(t);
	t.cur_tarTime_sec = static_cast<int>(time(NULL));
	t.restart = restart;
	trans_command(&t, sizeof(t), id);
	return;
}

void Emmiter::set_tar(const int& id, const int& sec, const int& nsec,
		int restart) {
	timings_meas t;
	init_timings_meas(t);
	t.tarTime_sec = sec;
	t.tarTime_nsec = nsec;
	t.restart = restart;
	trans_command(&t, sizeof(t), id);
	return;
}

void Emmiter::set_addtADC1(const int& id, const int& nsec, int restart) {
	timings_meas t;
	init_timings_meas(t);
	t.addtADC1 = nsec;
	t.restart = restart;
	trans_command(&t, sizeof(t), id);
	return;
}

void Emmiter::set_addtOfHyd(const int& id, const int& nsec, int restart) {
	timings_meas t;
	init_timings_meas(t);
	t.addtOfHyd = nsec;
	t.restart = restart;
	trans_command(&t, sizeof(t), id);
	return;
}

void Emmiter::set_subtOnAmp(const int& id, const int& nsec, int restart) {
	timings_meas t;
	init_timings_meas(t);
	t.subtOnAmp = nsec;
	t.restart = restart;
	trans_command(&t, sizeof(t), id);
	return;
}

void Emmiter::set_w_addtOfHyd(const int& id, const int& nsec, int restart) {
	timings_meas t;
	init_timings_meas(t);
	t.w_addtOfHyd = nsec;
	t.restart = restart;
	trans_command(&t, sizeof(t), id);
	return;
}

void Emmiter::set_w_subtOnAmp(const int& id, const int& nsec, int restart) {
	timings_meas t;
	init_timings_meas(t);
	t.w_subtOnAmp = nsec;
	t.restart = restart;
	trans_command(&t, sizeof(t), id);
	return;
}

void Emmiter::receive_ans(const void* pbuf, const size_t& size) {
	const int* ans = reinterpret_cast<const int*>(pbuf);
	switch (ans[0]) {
	case ENABLE_DEBUG:
		if (size == 2 * sizeof(int))
			cout << name_
					<< ": Команда позволение передачи отладочной информации "
					<< (ans[1] == OK ? "удовлетворена" : "отклонена") << endl;
		break;
	case GET_ENABLE_DEBUG:
		if (size == 3 * sizeof(int))
			cout << name_
					<< ": Параметры  передачи отладочной информации: период = "
					<< ans[2] << "мс, количество повторений = "
					<< (ans[1] == -1 ?
							"неограниченное количество" : std::to_string(ans[1]))
					<< "\n";
		break;
	case DISABLE_DEBUG:
		if (size == 2 * sizeof(int))
			cout << name_
					<< ": Команда запрещение передачи отладочной информации "
					<< (ans[1] == OK ? "удовлетворена" : "отклонена") << endl;
		break;
	case STOP_MEAS:
		if (size == 2 * sizeof(int))
			cout << name_ << ": Команда остановки процесса измерения "
					<< (ans[1] == OK ? "удовлетворена" : "отклонена") << endl;
		break;
	case START_MEAS:
		if (size == 2 * sizeof(int))
			cout << name_ << ": Команда запуска процесса измерения "
					<< (ans[1] == OK ? "удовлетворена" : "отклонена") << endl;
		break;
	case SET_TIMINGS:
		if (size == 2 * sizeof(int))
			cout << name_
					<< ": Команда изменения временных параметров процесса измерения "
					<< (ans[1] == OK ? "удовлетворена" : "отклонена") << endl;
		break;
	case GET_TIMINGS:
		if (size == sizeof(int) + sizeof(timings_meas)) {
			const timings_meas* pt = reinterpret_cast<const timings_meas*>(ans + 1);
			cout << name_ << ": Временные параметры процесса измерения: "
					"Период повторения: " << pt->tarTime_sec << " сек "
					<< pt->cur_tarTime_nsec << " нс" << endl
					<< " Время перед включением усилителя: " << pt->subtOnAmp
					<< " мс" << endl
					<< " Продолжительность импульса включения усилителя: "
					<< pt->w_subtOnAmp << " мс" << endl
					<< " Время до включения шунтирования гидрофона: "
					<< pt->addtOfHyd << " мс" << endl
					<< "Продолжительность импульса шунтирования гидрофона: "
					<< pt->w_subtOnAmp << endl;
		}
		break;
	case DEBUG_INFO:
		if (size == sizeof(int) + sizeof(d_info)) {
			const d_info* pdi = reinterpret_cast<const d_info*>(ans + 1);
			std::cout << "\tРЕГИСТРЫ ФИТЕРА" << "\n\t\t";
			std::cout << "PTP_CTL = " << std::dec << pdi->ptp_ctl << "   :   "
					<< std::hex << pdi->ptp_ctl << "\n\t\t";
			std::cout << "PTP_STS = " << std::dec << pdi->ptp_sts << "   :   "
					<< std::hex << pdi->ptp_sts << "\n\t\t";
			std::cout << "PTP_TSTS = " << std::dec << pdi->ptp_tsts << "   :   "
					<< std::hex << pdi->ptp_tsts << "\n\t\t";
			std::cout << "PTP_RATE = " << std::dec << pdi->ptp_rate << "   :   "
					<< std::hex << pdi->ptp_rate << "\n\t\t";
			std::cout << "PTP_TRIG0 = " << std::dec << pdi->ptp_trig0
					<< "   :   " << std::hex << pdi->ptp_trig0 << "\n\t\t";
			std::cout << "PTP_TRIG0_SEC = " << std::dec << pdi->ptp_trig0_sec
					<< "   :   " << std::hex << pdi->ptp_trig0_sec << "\n\t\t";
			std::cout << "PTP_TRIG0_NSEC = " << std::dec << pdi->ptp_trig0_nsec
					<< "   :   " << std::hex << pdi->ptp_trig0_nsec << "\n\t\t";
			std::cout << "PTP_TRIG0_PULS = " << std::dec << pdi->ptp_trig0_puls
					<< "   :   " << std::hex << pdi->ptp_trig0_puls << "\n\t\t";
			std::cout << "PTP_TRIG2 = " << std::dec << pdi->ptp_trig2
					<< "   :   " << std::hex << pdi->ptp_trig2 << "\n\t\t";
			std::cout << "PTP_TRIG2_SEC = " << std::dec << pdi->ptp_trig2_sec
					<< "   :   " << std::hex << pdi->ptp_trig2_sec << "\n\t\t";
			std::cout << "PTP_TRIG2_NSEC = " << std::dec << pdi->ptp_trig2_nsec
					<< "   :   " << std::hex << pdi->ptp_trig2_nsec << "\n\t\t";
			std::cout << "PTP_TRIG2_PULS = " << std::dec << pdi->ptp_trig2_puls
					<< "   :   " << std::hex << pdi->ptp_trig2_puls << "\n\t\t";
			std::cout << "PTP_TRIG6 = " << std::dec << pdi->ptp_trig6
					<< "   :   " << std::hex << pdi->ptp_trig6 << "\n\t\t";
			std::cout << "PTP_TRIG6_SEC = " << std::dec << pdi->ptp_trig6_sec
					<< "   :   " << std::hex << pdi->ptp_trig6_sec << "\n\t\t";
			std::cout << "PTP_TRIG6_NSEC = " << std::dec << pdi->ptp_trig6_nsec
					<< "   :   " << std::hex << pdi->ptp_trig6_nsec << "\n\t\t";
			std::cout << "PTP_TRIG6_PULS = " << std::dec << pdi->ptp_trig6_puls
					<< "   :   " << std::hex << pdi->ptp_trig6_puls << "\n\t\t";
			std::cout << "PTP_TRIG7 = " << std::dec << pdi->ptp_trig7
					<< "   :   " << std::hex << pdi->ptp_trig7 << "\n\t\t";
			std::cout << "PTP_TRIG7_SEC = " << std::dec << pdi->ptp_trig7_sec
					<< "   :   " << std::hex << pdi->ptp_trig7_sec << "\n\t\t";
			std::cout << "PTP_TRIG7_NSEC = " << std::dec << pdi->ptp_trig7_nsec
					<< "   :   " << std::hex << pdi->ptp_trig7_nsec << "\n\t\t";
			std::cout << "PTP_TRIG7_PULS = " << std::dec << pdi->ptp_trig7_puls
					<< "   :   " << std::hex << pdi->ptp_trig7_puls << "\n\t\t";
			std::cout << "PTP_TXCFG0 = " << std::dec << pdi->ptp_txcfg0
					<< "   :   " << std::hex << pdi->ptp_txcfg0 << "\n\t\t";
			std::cout << "PTP_TXCFG1 = " << std::dec << pdi->ptp_txcfg1
					<< "   :   " << std::hex << pdi->ptp_txcfg1 << "\n\t\t";
			std::cout << "PTP_RXCFG0 = " << std::dec << pdi->ptp_rxcfg0
					<< "   :   " << std::hex << pdi->ptp_rxcfg0 << "\n\t\t";
			std::cout << "PTP_RXCFG1 = " << std::dec << pdi->ptp_rxcfg1
					<< "   :   " << std::hex << pdi->ptp_rxcfg1 << "\n\t\t";
			std::cout << "PTP_RXCFG3 = " << std::dec << pdi->ptp_rxcfg3
					<< "   :   " << std::hex << pdi->ptp_rxcfg3 << "\n\t\t";
			std::cout << "PTP_RXCFG4 = " << std::dec << pdi->ptp_rxcfg4
					<< "   :   " << std::hex << pdi->ptp_rxcfg4 << "\n\t\t";
			std::cout << "PTP_COC = " << std::dec << pdi->ptp_coc << "   :   "
					<< std::hex << pdi->ptp_coc << "\n\t\t";
			std::cout << "PTP_CLKSRC = " << std::dec << pdi->ptp_clksrc
					<< "   :   " << std::hex << pdi->ptp_clksrc << "\n\t\t";
			std::cout << "PTP_GPIOMON = " << std::dec << pdi->ptp_gpiomon
					<< "   :   " << std::hex << pdi->ptp_gpiomon << "\n";
		}
		break;
	}
}

void Emmiter::receive_data(const void* pbuf, const size_t& size) {
	//проверка изменился ли месяц
	time_t tim = time(NULL);
	tm* t = localtime(&tim);
	if (id_month_ != t->tm_mon) {
		char m[10];
		strftime(m, sizeof(m), "%B", t);
		month_ = m;
		id_month_ = t->tm_mon;
		mkdir((root_dir_ + month_).c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
	}
	if (size < sizeof(head_data_adc))
		return;
	const head_data_adc* hdata = reinterpret_cast<const head_data_adc*>(pbuf);

	if (hdata->sec != cur_head.sec || hdata->nsec != cur_head.nsec
			|| hdata->hash != cur_head.hash) {
		cur_head = *hdata;
		//создание новой текущей папки
		cur_dir_ = root_dir_ + "/" + month_ + "/" + std::to_string(cur_head.sec)
				+ "_" + std::to_string(cur_head.nsec) + "_"
				+ std::to_string(cur_head.hash);
		mkdir(cur_dir_.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
	}
	std::ofstream file(cur_dir_ + "/" + std::to_string(id_),
			std::ios::out | std::ios::trunc | std::ios::binary);
	file.write(reinterpret_cast<const char*>(pbuf) + sizeof(head_data_adc),
			size - sizeof(head_data_adc));
	file.close();
}

void Emmiter::set_root_dir(const string& name) {
	root_dir_ = name;
	char m[10];
	time_t tim = time(NULL);
	tm* t = localtime(&tim);
	strftime(m, sizeof(m), "%B", t);
	month_ = m;
	id_month_ = t->tm_mon;
	mkdir((root_dir_ + "/" + month_).c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
	return;
}

Emmiter::~Emmiter() {
}

} /* namespace mpce_n */
