#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "pcap.h"
#include <stdbool.h>
#include <time.h>
#include <string.h>

//字符所对应ASCII码
#define U 0x55
#define S 0x53
#define E 0x45
#define R 0x52

#define P 0x50
#define A 0x41
//过滤器
#define FILTER "tcp"

//CSV文档相对目录
#define CSV_FILE_PATH "log.csv"

//MAC地址
typedef struct MAC_ADDR {
	u_char byte0;
	u_char byte1;
	u_char byte2;
	u_char byte3;
	u_char byte4;
	u_char byte5;
}MAC_ADDR;

//IP地址
typedef struct IP_ADDR {
	u_char byte0;
	u_char byte1;
	u_char byte2;
	u_char byte3;
}IP_ADDR;

//判断两MAC地址是否相等
bool MAC_EQU(MAC_ADDR* MAC_ADDR_0, MAC_ADDR* MAC_ADDR_1) {
	return
		(MAC_ADDR_0->byte0 == MAC_ADDR_1->byte0) &&
		(MAC_ADDR_0->byte1 == MAC_ADDR_1->byte1) &&
		(MAC_ADDR_0->byte2 == MAC_ADDR_1->byte2) &&
		(MAC_ADDR_0->byte3 == MAC_ADDR_1->byte3) &&
		(MAC_ADDR_0->byte4 == MAC_ADDR_1->byte4) &&
		(MAC_ADDR_0->byte5 == MAC_ADDR_1->byte5);
}

//判断两IP地址是否相等
bool IP_EQU(IP_ADDR* IP_ADDR_0, IP_ADDR* IP_ADDR_1) {
	return
		(IP_ADDR_0->byte0 == IP_ADDR_1->byte0) &&
		(IP_ADDR_0->byte1 == IP_ADDR_1->byte1) &&
		(IP_ADDR_0->byte2 == IP_ADDR_1->byte2) &&
		(IP_ADDR_0->byte3 == IP_ADDR_1->byte3);
}

//握手状态
typedef enum ConnState {
	SEND_USER,				//发送用户名
	RESP_USER,				//回复用户名
	SEND_PASS,				//发送密码
	RESP_PASS,				//回复密码
}ConnState;

//CSV记录所需项
typedef struct CSV_FORMAT {
	struct tm* TIMESTAMP;	//时间戳
	int TV_USEC;			//时间戳（毫秒部分）
	MAC_ADDR Client_MAC;	//用户MAC地址
	IP_ADDR Client_IP;		//用户IP地址
	MAC_ADDR FTP_MAC;		//FTP MAC地址
	IP_ADDR FTP_IP;			//FTP IP地址
	char* USER;				//用户
	char* PASS;				//密码
	boolean isSuccess;		//连接是否成功
	char* msg;				//连接信息
}CSV_FORMAT;

//设备列表
pcap_if_t* all_devs;

//握手状态
ConnState state = SEND_USER;

//输出到.csv文件里的东西
CSV_FORMAT csv;

//输出文件
FILE* fp = NULL;

//获取设备列表（返回设备列表）
pcap_if_t* getAllDevs() {
	char error[PCAP_ERRBUF_SIZE];
	if (pcap_findalldevs(&all_devs, error) == -1) {
		printf("寻找设备时发生错误: %s\n", error);
		exit(-1);
	}
	return all_devs;
}

//输出设备列表（返回列表长度）
int printAllDevs() {
	int DevsCount = 0;
	for (pcap_if_t* d = all_devs; d; d = d->next) {
		printf("%d. %s", ++DevsCount, d->name);
		if (d->description)
			printf(" (%s)\n", d->description);
		else
			printf(" (No description available)\n");
	}
	return DevsCount;
}

//选择设备（返回设备信息）
pcap_if_t* selectDev(int DevsCount) {
	int select_dev_index;
	printf("请输入设备序号(1-%d):", DevsCount);
	scanf("%d", &select_dev_index);
	if (select_dev_index <= 0 || select_dev_index > DevsCount) {
		printf("设备号应在(1-%d)间，实际输入为%d，输入超限！\n", DevsCount, select_dev_index);
		pcap_freealldevs(all_devs);//释放设备列表
		exit(-1);
	}
	pcap_if_t* current_dev;

	//定位到该设备
	int temp_index = 0;
	for (current_dev = all_devs; temp_index < select_dev_index - 1; current_dev = current_dev->next, temp_index++);
	return current_dev;
}

//获取句柄
pcap_t* getHandle(pcap_if_t* dev) {
	pcap_t* handle;
	char error[PCAP_ERRBUF_SIZE];

	//打开接口
	if ((handle = pcap_open_live(dev->name, 65536, 1, 1000, error)) == NULL) {
		printf("未能打开接口适配器，WinPcap不支持%s", dev->name);
		pcap_freealldevs(all_devs);
		exit(-1);
	}

	//检查是否在以太网
	if (pcap_datalink(handle) != DLT_EN10MB) {
		printf("此程序只在以太网网络上工作！\n");
		pcap_freealldevs(all_devs);
		exit(-1);
	}
	return handle;
}

//设置过滤器
void setfilter(pcap_t* handle, u_int netmask) {
	struct bpf_program fcode;

	//检查过滤器格式
	if (pcap_compile(handle, &fcode, FILTER, 1, netmask) < 0) {
		printf("过滤器格式错误！\n");
		pcap_freealldevs(all_devs);
		exit(-1);
	}

	//设置过滤器
	if (pcap_setfilter(handle, &fcode) < 0) {
		printf("设置过滤器时出错！\n");
		pcap_freealldevs(all_devs);
		exit(-1);
	}
}

//从帧中读取字符串（仅限以0x0D 0x0A结尾的帧）
char* read_msg(u_char* head) {
	int index = 0, msglen = 0;
	char* msg = (char*)malloc(sizeof(char));
	while (head[index] != 0x0D && head[index + 1] != 0x0A) {
		++msglen;
		msg = (char*)realloc(msg, msglen);
		msg[index] = head[index];
		++index;
	}
	msg = (char*)realloc(msg, msglen + 1);
	msg[msglen] = '\0';
	return msg;
}

//MAC地址转字符串
char* MAC_to_str(MAC_ADDR* mac) {
	char* mac_str = (char*)malloc(18 * sizeof(char));
	sprintf(mac_str, "%02X-%02X-%02X-%02X-%02X-%02X",
		mac->byte0,
		mac->byte1,
		mac->byte2,
		mac->byte3,
		mac->byte4,
		mac->byte5);
	return mac_str;
}

//IP地址转字符串
char* IP_to_str(IP_ADDR* ip) {
	char* ip_str = (char*)malloc(16 * sizeof(char));
	sprintf(ip_str, "%3d:%3d:%3d:%3d",
		ip->byte0,
		ip->byte1,
		ip->byte2,
		ip->byte3);
	return ip_str;
}

//在字符串两边加双引号（适应csv输出）
char* add_quotes(char* str) {
	char* result = (char*)malloc((strlen(str) + 3) * sizeof(char));
	strcpy(&result[1], str);
	result[0] = '\"';
	result[strlen(str) + 1] = '\"';
	result[strlen(str) + 2] = '\0';
	return result;
}

//生成用于输出和写入CSV的字符串
char* CSVFORMAT_to_str() {
	//各种变量转字符串
	char* timestr = (char*)malloc(20 * sizeof(char));
	strftime(timestr, 20 * sizeof(char), "%Y/%m/%d %H:%M:%S", csv.TIMESTAMP);
	char* Client_MAC_str = MAC_to_str(&csv.Client_MAC);
	char* Client_IP_str = IP_to_str(&csv.Client_IP);
	char* FTP_MAC_str = MAC_to_str(&csv.FTP_MAC);
	char* FTP_IP_str = IP_to_str(&csv.FTP_IP);
	char* result = (char*)malloc(256 * sizeof(char));

	//输出转换后的字符串
	sprintf(result, "%s:%06d,%s,%s,%s,%s,%s,%s,%s,%s\n",
		timestr,//时间戳 20字节
		csv.TV_USEC,//时间戳:毫秒 6字节
		Client_MAC_str,//本机MAC 18字节
		Client_IP_str,//本机IP 16字节
		FTP_MAC_str,//FTPMAC 18字节
		FTP_IP_str,//FTPIP 16字节
		add_quotes(csv.USER),
		add_quotes(csv.PASS),
		csv.isSuccess ? "SUCCESS" : "FAILED",
		add_quotes(csv.msg));
	return result;
}

//向文件中写入信息
void write_str_to_file(char* str) {
	fp = fopen(CSV_FILE_PATH, "a");
	if (fp) {
		fprintf(fp, "%s", str);
		fclose(fp);
	}
	else printf("输出文档被占用！写入失败");
}

//从收到的数据提取出用于输出和写入CSV的数据
void pkt_to_csv(const struct pcap_pkthdr* header, const u_char* pkt_data) {
	time_t local_tv_sec = header->ts.tv_sec;
	u_char flag = pkt_data[0x2F];
	switch (state)
		//发送用户名
		case SEND_USER: {
		csv.TIMESTAMP = localtime(&local_tv_sec);
		csv.TV_USEC = header->ts.tv_usec;
		if (flag == 0x18 &&//确认标志位为0x18 [PSH, ACK] 下同
			pkt_data[0x36] == U && pkt_data[0x37] == S &&
			pkt_data[0x38] == E && pkt_data[0x39] == R) {//0x36~0x39为USER四个字符

			//记录FTP和用户的MAC和IP
			csv.Client_MAC = *(MAC_ADDR*)(&pkt_data[0x06]);
			csv.Client_IP = *(IP_ADDR*)(&pkt_data[0x1A]);
			csv.FTP_MAC = *(MAC_ADDR*)(&pkt_data[0x00]);
			csv.FTP_IP = *(IP_ADDR*)(&pkt_data[0x1E]);
			
			//记录并输出用户名
			csv.USER = read_msg(&pkt_data[0x3B]);
			printf("USERNAME:%s\n", csv.USER);

			//进入接受返回状态
			state = RESP_USER;
		}
		break;

		//接受用户名发送是否成功返回信息
		case RESP_USER: {
			if (flag == 0x18) {
				if (MAC_EQU(&csv.FTP_MAC, (MAC_ADDR*)(&pkt_data[0x06])) &&//确认IP和MAC正确
					IP_EQU(&csv.FTP_IP, (IP_ADDR*)(&pkt_data[0x1A])) &&
					MAC_EQU(&csv.Client_MAC, (MAC_ADDR*)(&pkt_data[0x00])) &&
					IP_EQU(&csv.Client_IP, (IP_ADDR*)(&pkt_data[0x1E]))) {
					if (pkt_data[0x36] == 0x33 && pkt_data[0x37] == 0x33 && pkt_data[0x38] == 0x31) {//需要密码
						state = SEND_PASS;
					}
					else {//无需密码
						//回到初始状态
						state == SEND_USER;

						//csv中写入消息
						csv.msg = read_msg(&pkt_data[0x36]);

						//保存csv
						write_str_to_file(CSVFORMAT_to_str());

						//清空csv
						memset(&csv, 0, sizeof(CSV_FORMAT));
					}
					printf("%s\n", read_msg(&pkt_data[0x36]));
				}
			}
			break;
		}

		//发送密码
		case SEND_PASS: {
			if (flag == 0x18) {
				if (MAC_EQU(&csv.Client_MAC, (MAC_ADDR*)(pkt_data + 0x06)) &&
					IP_EQU(&csv.Client_IP, (IP_ADDR*)(pkt_data + 0x1A)) &&
					MAC_EQU(&csv.FTP_MAC, (MAC_ADDR*)(pkt_data)) &&
					IP_EQU(&csv.FTP_IP, (IP_ADDR*)(pkt_data + 0x1E)) &&
					pkt_data[0x36] == P && pkt_data[0x37] == A &&
					pkt_data[0x38] == S && pkt_data[0x39] == S) {
					//记录并输出密码
					csv.PASS = read_msg(&pkt_data[0x3B]);
					printf("PASSWORD:%s\n", csv.PASS);
				}
				state = RESP_PASS;
			}
			break;
		}
		
		//接受密码是否正确返回信息
		case RESP_PASS: {
			if (flag == 0x18) {
				if (MAC_EQU(&csv.FTP_MAC, (MAC_ADDR*)(pkt_data + 0x06)) &&
					IP_EQU(&csv.FTP_IP, (IP_ADDR*)(pkt_data + 0x1A)) &&
					MAC_EQU(&csv.Client_MAC, (MAC_ADDR*)(pkt_data)) &&
					IP_EQU(&csv.Client_IP, (IP_ADDR*)(pkt_data + 0x1E))) {
					if (pkt_data[0x36] == 0x35 && pkt_data[0x37] == 0x33 && pkt_data[0x38] == 0x30) {//530 密码不正确
						csv.isSuccess = false;
					}
					else if (pkt_data[0x36] == 0x32 && pkt_data[0x37] == 0x33 && pkt_data[0x38] == 0x30) {//230 密码正确
						csv.isSuccess = true;
					}
					csv.msg = read_msg(&pkt_data[0x36]);
					printf("%s\n\n", csv.msg);
					
					//保存CSV
					write_str_to_file(CSVFORMAT_to_str());
				}
				state = SEND_USER;

				//清空csv
				memset(&csv, 0, sizeof(CSV_FORMAT));
			}
			break;
		}
	}
}

//监听事件
void packet_handler(u_char* param, const struct pcap_pkthdr* header, const u_char* pkt_data) {
	(VOID)(param);
	pkt_to_csv(header, pkt_data);
}

//主函数
int main() {
	//清空文件
	fp = fopen(CSV_FILE_PATH, "w");
	if (fp) {
		fprintf(fp, "TimeStamp,Client MAC,Client IP,FTP MAC,FTP IP,Username,Password,Statement,Message\n");
		fclose(fp);
	}
	else {
		printf("输出文档被占用！程序结束");
		system("pause");
		return 0;
	}
	//设备选择
	pcap_if_t* alldevs = getAllDevs();//获取设备列表
	int DevsCount = printAllDevs();//输出设备列表
	if (DevsCount == 0) {
		printf("\n未发现设备！请确认WinPcap已经安装！\n");
		system("pause");
		return -1;
	}
	pcap_if_t* current_dev = selectDev(DevsCount);//选择设备

	//获取句柄
	pcap_t* handle = getHandle(current_dev);

	//设置掩码
	u_int netmask;
	if (current_dev->addresses != NULL)//当前设备地址不为空则取掩码
		netmask = ((struct sockaddr_in*)(current_dev->addresses->netmask))->sin_addr.S_un.S_addr;
	else netmask = 0xffffff;//假设设备在C类以太网上运行，掩码为0xFFFFFF

	//过滤器
	setfilter(handle, netmask);

	//监听准备
	printf("开始监听:%s\n", current_dev->description);
	pcap_freealldevs(alldevs);//释放设备列表

	//开始监听
	pcap_loop(handle, 0, packet_handler, NULL);

	return 0;
}