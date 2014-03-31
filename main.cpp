#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include "Emmiter.h"
#include <map>
#include <vector>
#include <algorithm>

#define MAX_SIZE_SAMPL_REC 4100 //определяет длину буфера передачи сокета
#define EMMITER_ADDR	"192.168.100.29"
#define CENTRAL_PORT	31000
#define IN_PORT			31000

using std::string;
using std::cerr;
using std::cout;
using std::cin;
using mpce_n::Emmiter;

static struct collector { //структура, используемая для аккумулироания данных
	Emmiter::head head; //шапка раннее принятого пакета
	std::size_t total_size; //общий размер уже принятых данных
	std::vector<char> data; //данные
} col;

const char* NameMaster = "mas";
const char* NameSlave = "slav";
const char* NameAll = "all";

static const unsigned InitSizeVectorCollector = 6000; //начальный размер вектора данных коллектора
const char* fill_collector(collector& c, const char* buf,
		const std::size_t& size); /*функция пополняющая коллектор данными; возвращает начальный адрес данных,
		 если коллектор полон, иначе nullptr*/
void init_collector(collector& c); //инициализация коллектора
void hand_command_line(const std::map<string, Emmiter*>& e);
void hand_receiv(std::map<string, Emmiter*>& e, const char* buf,
		const std::size_t& size);

int main(int argc, char* argv[]) {
	string name_root_dir;
	int sock;
	sockaddr_in addrCen;
	std::map<string, Emmiter*> emSet;
	int recBuf[MAX_SIZE_SAMPL_REC * 4]; //приёмный буфер
	if (argc != 2) {
		cout << "неверное количество аргументов командной строки\n";
		return -1;
	} else
		name_root_dir = argv[1];
	Emmiter::set_root_dir(name_root_dir);
	bzero(&addrCen, sizeof(addrCen));
	addrCen.sin_family = AF_INET;
	addrCen.sin_port = htons(CENTRAL_PORT);
	addrCen.sin_addr.s_addr = htonl(INADDR_ANY);
	//создание сокета
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		cerr << "создать сокет не удалось\n";
		exit(1);
	}
	if (bind(sock, reinterpret_cast<sockaddr*>(&addrCen), sizeof(addrCen))) {
		cerr << "не удалось связать сокет с адресом\n";
		exit(1);
	}
	int sizeRec = MAX_SIZE_SAMPL_REC * 4 * sizeof(short);
	if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &sizeRec, sizeof(int)) == -1) {
		cerr << "Не поддерживается объём буфера приёма сокета в размере "
				<< sizeRec << " байт\n";
		exit(1);
	}
	//инициализация коллектора
	init_collector(col);
	//инициализация набора эммитеров
	Emmiter::set_ethernet_configuration(sock, addrCen);
	emSet.insert(
			std::make_pair(NameMaster, new Emmiter(Emmiter::MASTER, "master")));
	emSet.insert(
			std::make_pair(NameSlave, new Emmiter(Emmiter::SLAVE, "slave")));
	fd_set fdin; //набор дескрипторов, на которых ожидаются входные данные
	int status = 0;

	for (;;) {
		FD_ZERO(&fdin);
		FD_SET(STDIN_FILENO, &fdin);
		FD_SET(sock, &fdin);
		//ожидание событий
		status = select(sock + 1, &fdin, NULL, NULL, NULL);
		if (status == -1) {
			if (errno == EINTR)
				continue;
			else {
				perror("Функция select завершилась крахом\n");
				exit(1);
			}
		}
		if (FD_ISSET(STDIN_FILENO, &fdin))
			hand_command_line(emSet);
		if (FD_ISSET(sock, &fdin)) {
			sockaddr_in srcAddr;
			size_t size = sizeof(srcAddr);
			unsigned len = recvfrom(sock, reinterpret_cast<void *>(&recBuf),
					sizeof(recBuf), 0, reinterpret_cast<sockaddr*>(&srcAddr),
					&size);
			if (len > sizeof(Emmiter::head))
				hand_receiv(emSet, reinterpret_cast<char*>(recBuf), len);
		}
	}
	return 0;
}

void hand_command_line(const std::map<string, Emmiter*>& e) {
	std::istringstream message;
	string command, mes, det;
	int id;
	getline(cin, command);
	message.str(command);
	message >> mes;
	if (e.count(mes)) {
		auto x = e.find(mes);
		id = x->second->id_;
	} else if (det == NameAll)
		id = Emmiter::allId_;
	else {
		std::cout << "Пункта назначения " << mes << " не существует\n";
		return;
	}
	message >> mes;
	if (mes == "e_d") {
		int num = -1;
		unsigned time;
		message >> mes;
		if (mes != "n")
			num = stoi(mes);
		message >> mes;
		time = stoi(mes);
		Emmiter::enable_debug(num, time, id);
	} else if (mes == "d_d")
		Emmiter::disable_debug(id);
	else if (mes == "g_e_d")
		Emmiter::get_enable_debug(id);
	else if (mes == "s_adc") {
		int restart = -1;
		message >> mes;
		int time = stoi(mes);
		message >> mes;
		if (mes == "r")
			restart = 1;
		Emmiter::set_addtADC1(id, time, restart);
	} else if (mes == "s_hyd") {
		int restart = -1;
		message >> mes;
		int time = stoi(mes);
		message >> mes;
		if (mes == "r")
			restart = 1;
		Emmiter::set_addtOfHyd(id, time, restart);
	} else if (mes == "s_amp") {
		int restart = -1;
		message >> mes;
		int time = stoi(mes);
		message >> mes;
		if (mes == "r")
			restart = 1;
		Emmiter::set_subtOnAmp(id, time, restart);
	} else if (mes == "sync") {
		int restart = -1;
		message >> mes;
		if (mes == "r")
			restart = 1;
		Emmiter::set_sync(id, restart);
	} else if (mes == "s_tar") {
		int restart = -1;
		message >> mes;
		int s = stoi(mes);
		message >> mes;
		int ns = stoi(mes);
		if (mes == "r")
			restart = 1;
		Emmiter::set_tar(id, s, ns, restart);
	} else if (mes == "s_w_hyd") {
		int restart = -1;
		message >> mes;
		int time = stoi(mes);
		message >> mes;
		if (mes == "r")
			restart = 1;
		Emmiter::set_w_addtOfHyd(id, time, restart);
	} else if (mes == "s_w_amp") {
		int restart = -1;
		message >> mes;
		int time = stoi(mes);
		message >> mes;
		if (mes == "r")
			restart = 1;
		Emmiter::set_w_subtOnAmp(id, time, restart);
	} else if (mes == "start")
		Emmiter::start_measurement(id);
	else if (mes == "stop")
			Emmiter::stop_measurement(id);
	return;
}

const char* fill_collector(collector& c, const char* pbuf,
		const std::size_t& size) {
	if (size < sizeof(Emmiter::head))
		return nullptr;
	const Emmiter::head* h = reinterpret_cast<const Emmiter::head*>(pbuf);
	const char* pdata = pbuf + sizeof(Emmiter::head);
	c.head.size = h->size;
	++c.head.count;
	switch (h->count) {
	case 0:
		std::copy(pdata, pdata + h->size, c.data.begin());
		c.head = *h;
		c.total_size = h->size;
		break;
	case Emmiter::LAST:
		c.head.count = Emmiter::LAST;
		if (c.head != *h) {
			std::copy(pdata, pdata + h->size, c.data.begin());
			c.head = *h;
			c.total_size = h->size;
			return c.data.data();
		} else {
			std::copy(pdata, pdata + h->size, c.data.begin() + c.total_size);
			c.total_size += h->size;
			return c.data.data();
		}
		break;
	default:
		c.head.size = h->size;
		if (c.head == *h) {
			std::copy(pdata, pdata + h->size, c.data.begin() + c.total_size);
			c.total_size += h->size;
		}
		break;
	}
	return nullptr;
}

inline void init_collector(collector& c) {
	c.total_size = 0;
	c.head.count = Emmiter::LAST;
	c.head.size = 0;
	c.data.resize(InitSizeVectorCollector);
	return;
}

void hand_receiv(std::map<string, Emmiter*>& e, const char* pbuf,
		const std::size_t& size) {
	const char *pdata = fill_collector(col, pbuf, size);
	if (pdata == nullptr)
		return;
	const Emmiter::head* h = reinterpret_cast<const Emmiter::head*>(pbuf);
	if (h->type == Emmiter::DATA) {
		if (h->src & Emmiter::MASTER)
			e[NameMaster]->receive_data(pdata, col.total_size);
		if (h->src & Emmiter::SLAVE)
			e[NameSlave]->receive_data(pdata, col.total_size);
	} else if (h->type == Emmiter::ANSWER) {
		if (h->src & Emmiter::MASTER)
			e[NameMaster]->receive_ans(pdata, col.total_size);
		if (h->src & Emmiter::SLAVE)
			e[NameSlave]->receive_ans(pdata, col.total_size);
	}
	return;
}
