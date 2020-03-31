#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "pcap.h"
#include <stdbool.h>
#include <time.h>

//过滤器
#define FILTER "ip and udp"

//报警下限（单位 B）
#define WARNING_LOWERLIMIT 1048576//1024*1024

//统计数据间隔（单位 ms）
#define STATEMENT_OUTPUT_INTERVAL 5000

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

//CSV记录所需项
typedef struct CSV_FORMAT{
	struct tm* TIMESTAMP;	//时间戳
	int TV_USEC;			//时间戳（毫秒部分）
	MAC_ADDR SRC_MAC;		//源MAC地址
	IP_ADDR SRC_IP;			//源IP地址
	MAC_ADDR DES_MAC;		//目的MAC地址
	IP_ADDR DES_IP;			//目的IP地址
	int LEN;				//帧长度
}CSV_FORMAT;

//流量记录表的某一行
typedef struct FLOW_STATEMENT {
	MAC_ADDR MAC;		//源/目的MAC地址
	IP_ADDR IP;			//源/目的IP地址
	unsigned TOL_FLOW;	//一段时间内的总流量
}FLOW_STATEMENT;

//流量记录表
typedef struct FLOW_STATEMENT_LIST {
	FLOW_STATEMENT* HEAD;//表头
	int length;//表长
}FLOW_STATEMENT_LIST;

//设备列表
pcap_if_t* all_devs;

//输出文件
FILE* fp = NULL;

//流量记录表
FLOW_STATEMENT_LIST* flow_alarm_list;
FLOW_STATEMENT_LIST* flow_recv_list;
FLOW_STATEMENT_LIST* flow_send_list;

//警报间隔计时tickcount
ULONGLONG last_sec;

//本设备MAC地址
MAC_ADDR self_mac_addr;

//统计表输出计时
ULONGLONG last_recv_output;
ULONGLONG last_send_output;

//获取设备列表（返回设备列表）
pcap_if_t* getAllDevs() {
	char error[PCAP_ERRBUF_SIZE];
	if (pcap_findalldevs(&all_devs, error) == -1){
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
	if (select_dev_index <= 0 || select_dev_index > DevsCount){
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
	if (pcap_datalink(handle) != DLT_EN10MB){
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

//从收到的数据提取出用于输出和写入CSV的数据
CSV_FORMAT* pkt_to_csv(const struct pcap_pkthdr* header, const u_char* pkt_data) {
	struct CSV_FORMAT *csv_format=(struct CSV_FORMAT*)malloc(sizeof(struct CSV_FORMAT));
	if (csv_format == NULL)exit(-1);
	time_t local_tv_sec = header->ts.tv_sec;

	csv_format->TIMESTAMP = localtime(&local_tv_sec);
	csv_format->TV_USEC = header->ts.tv_usec;
	csv_format->SRC_MAC = *(MAC_ADDR*)(pkt_data + 0x06);
	csv_format->SRC_IP = *(IP_ADDR*)(pkt_data + 0x1A);
	csv_format->DES_MAC = *(MAC_ADDR*)(pkt_data);
	csv_format->DES_IP = *(IP_ADDR*)(pkt_data + 0x1E);
	csv_format->LEN = header->len;
	return csv_format;
}

//生成用于输出和写入CSV的字符串
char* CSVFORMAT_to_str(CSV_FORMAT* csv_format) {
	char timestr[20];
	strftime(timestr, sizeof(timestr), "%Y/%m/%d %H:%M:%S", csv_format->TIMESTAMP);
	char src_mac_str[18];
	char src_ip_str[16];
	char des_mac_str[18];
	char des_ip_str[16];
	sprintf(src_mac_str, "%02X-%02X-%02X-%02X-%02X-%02X",
		csv_format->SRC_MAC.byte0,
		csv_format->SRC_MAC.byte1,
		csv_format->SRC_MAC.byte2,
		csv_format->SRC_MAC.byte3,
		csv_format->SRC_MAC.byte4,
		csv_format->SRC_MAC.byte5);
	sprintf(src_ip_str, "%3d:%3d:%3d:%3d",
		csv_format->SRC_IP.byte0,
		csv_format->SRC_IP.byte1,
		csv_format->SRC_IP.byte2,
		csv_format->SRC_IP.byte3);
	sprintf(des_mac_str, "%02X-%02X-%02X-%02X-%02X-%02X",
		csv_format->DES_MAC.byte0,
		csv_format->DES_MAC.byte1,
		csv_format->DES_MAC.byte2,
		csv_format->DES_MAC.byte3,
		csv_format->DES_MAC.byte4,
		csv_format->DES_MAC.byte5);
	sprintf(des_ip_str, "%3d:%3d:%3d:%3d",
		csv_format->DES_IP.byte0,
		csv_format->DES_IP.byte1,
		csv_format->DES_IP.byte2,
		csv_format->DES_IP.byte3);
	char result[100];
	sprintf(result, "%s+%06d,%s,%s,%s,%s,%d",
		timestr,
		csv_format->TV_USEC,
		src_mac_str,
		src_ip_str,
		des_mac_str,
		des_ip_str,
		csv_format->LEN);
	return result;
}

//流量统计
void add_alarm_flow(MAC_ADDR* mac_addr, IP_ADDR* ip_addr, int flow) {
	//流量重置
	if (GetTickCount64() - last_sec >= 1000) {
		for (int i = 0; i < flow_alarm_list->length; ++i) {
			(flow_alarm_list->HEAD + i)->TOL_FLOW = 0;
		}
		last_sec = GetTickCount64();
	}

	//流量统计
	bool isFound = false;
	for (int i = 0; i < flow_alarm_list->length; ++i) {
		if (MAC_EQU(mac_addr, &(flow_alarm_list->HEAD + i)->MAC) && 
			IP_EQU(ip_addr, &(flow_alarm_list->HEAD + i)->IP)) {
			(flow_alarm_list->HEAD + i)->TOL_FLOW += flow;
			isFound = true;
		}
	}
	if (!isFound) {
		++(flow_alarm_list->length);
		if ((flow_alarm_list->HEAD = (FLOW_STATEMENT*)realloc(flow_alarm_list->HEAD, (flow_alarm_list->length) * sizeof(FLOW_STATEMENT))) == NULL)exit(-1);		
		if ((flow_alarm_list->HEAD + flow_alarm_list->length - 1) == NULL)exit(-1);
		(flow_alarm_list->HEAD + flow_alarm_list->length - 1)->MAC = *mac_addr;
		(flow_alarm_list->HEAD + flow_alarm_list->length - 1)->IP = *ip_addr;
		(flow_alarm_list->HEAD + flow_alarm_list->length - 1)->TOL_FLOW = flow;
	}

	//流量报警
	for (int i = 0; i < flow_alarm_list->length; ++i) {
		if ((flow_alarm_list->HEAD + i)->TOL_FLOW >= WARNING_LOWERLIMIT) {
			printf("来自%02X-%02X-%02X-%02X-%02X-%02X,%3d:%3d:%3d:%3d的流量超过限制！\n",
				(flow_alarm_list->HEAD + i)->MAC.byte0,
				(flow_alarm_list->HEAD + i)->MAC.byte1,
				(flow_alarm_list->HEAD + i)->MAC.byte2,
				(flow_alarm_list->HEAD + i)->MAC.byte3,
				(flow_alarm_list->HEAD + i)->MAC.byte4,
				(flow_alarm_list->HEAD + i)->MAC.byte5,
				(flow_alarm_list->HEAD + i)->IP.byte0,
				(flow_alarm_list->HEAD + i)->IP.byte1,
				(flow_alarm_list->HEAD + i)->IP.byte2,
				(flow_alarm_list->HEAD + i)->IP.byte3);
		}
	}
}

//接收统计
void add_recv_flow(MAC_ADDR* mac_addr, IP_ADDR* ip_addr, int flow) {
	//流量统计
	bool isFound = false;
	for (int i = 0; i < flow_recv_list->length; ++i) {
		if (MAC_EQU(mac_addr, &(flow_recv_list->HEAD + i)->MAC) &&
			IP_EQU(ip_addr, &(flow_recv_list->HEAD + i)->IP)) {
			(flow_recv_list->HEAD + i)->TOL_FLOW += flow;
			isFound = true;
		}
	}
	if (!isFound) {
		++(flow_recv_list->length);
		if ((flow_recv_list->HEAD = (FLOW_STATEMENT*)realloc(flow_recv_list->HEAD, (flow_recv_list->length) * sizeof(FLOW_STATEMENT))) == NULL)exit(-1);
		if ((flow_recv_list->HEAD + flow_recv_list->length - 1) == NULL)exit(-1);
		(flow_recv_list->HEAD + flow_recv_list->length - 1)->MAC = *mac_addr;
		(flow_recv_list->HEAD + flow_recv_list->length - 1)->IP = *ip_addr;
		(flow_recv_list->HEAD + flow_recv_list->length - 1)->TOL_FLOW = flow;
	}

	//统计输出
	if ((GetTickCount64() - last_recv_output) >= STATEMENT_OUTPUT_INTERVAL) {
		printf("=========================RECV FROM=========================\n");
		for (int i = 0; i < flow_recv_list->length; ++i) {
			printf("MAC:%02X-%02X-%02X-%02X-%02X-%02X, IP:%3d:%3d:%3d:%3d, TOTALFLOW:%d\n",
				(flow_recv_list->HEAD + i)->MAC.byte0,
				(flow_recv_list->HEAD + i)->MAC.byte1,
				(flow_recv_list->HEAD + i)->MAC.byte2,
				(flow_recv_list->HEAD + i)->MAC.byte3,
				(flow_recv_list->HEAD + i)->MAC.byte4,
				(flow_recv_list->HEAD + i)->MAC.byte5,
				(flow_recv_list->HEAD + i)->IP.byte0,
				(flow_recv_list->HEAD + i)->IP.byte1,
				(flow_recv_list->HEAD + i)->IP.byte2,
				(flow_recv_list->HEAD + i)->IP.byte3,
				(flow_recv_list->HEAD + i)->TOL_FLOW);
		}
		printf("=========================RECV FROM=========================\n");
		last_recv_output = GetTickCount64();
	}
}

//发送统计
void add_send_flow(MAC_ADDR* mac_addr, IP_ADDR* ip_addr, int flow) {
	//流量统计
	bool isFound = false;
	for (int i = 0; i < flow_send_list->length; ++i) {
		if (MAC_EQU(mac_addr, &(flow_send_list->HEAD + i)->MAC) &&
			IP_EQU(ip_addr, &(flow_send_list->HEAD + i)->IP)) {
			(flow_send_list->HEAD + i)->TOL_FLOW += flow;
			isFound = true;
		}
	}
	if (!isFound) {
		++(flow_send_list->length);
		if ((flow_send_list->HEAD = (FLOW_STATEMENT*)realloc(flow_send_list->HEAD, (flow_send_list->length) * sizeof(FLOW_STATEMENT))) == NULL)exit(-1);
		if ((flow_send_list->HEAD + flow_send_list->length - 1) == NULL)exit(-1);
		(flow_send_list->HEAD + flow_send_list->length - 1)->MAC = *mac_addr;
		(flow_send_list->HEAD + flow_send_list->length - 1)->IP = *ip_addr;
		(flow_send_list->HEAD + flow_send_list->length - 1)->TOL_FLOW = flow;
	}

	//统计输出
	if ((GetTickCount64() - last_send_output) >= STATEMENT_OUTPUT_INTERVAL) {
		printf("==========================SEND TO==========================\n");
		for (int i = 0; i < flow_send_list->length; ++i) {
			printf("MAC:%02X-%02X-%02X-%02X-%02X-%02X, IP:%3d:%3d:%3d:%3d, TOTALFLOW:%d\n",
				(flow_send_list->HEAD + i)->MAC.byte0,
				(flow_send_list->HEAD + i)->MAC.byte1,
				(flow_send_list->HEAD + i)->MAC.byte2,
				(flow_send_list->HEAD + i)->MAC.byte3,
				(flow_send_list->HEAD + i)->MAC.byte4,
				(flow_send_list->HEAD + i)->MAC.byte5,
				(flow_send_list->HEAD + i)->IP.byte0,
				(flow_send_list->HEAD + i)->IP.byte1,
				(flow_send_list->HEAD + i)->IP.byte2,
				(flow_send_list->HEAD + i)->IP.byte3,
				(flow_send_list->HEAD + i)->TOL_FLOW);
		}
		printf("==========================SEND TO==========================\n");
		last_send_output = GetTickCount64();
	}
}

//监听事件
void packet_handler(u_char* param, const struct pcap_pkthdr* header, const u_char* pkt_data){
	(VOID)(param);
	struct CSV_FORMAT* csv = pkt_to_csv(header, pkt_data);
	add_alarm_flow(&csv->SRC_MAC, &csv->SRC_IP, csv->LEN);
	add_recv_flow(&csv->DES_MAC, &csv->DES_IP, csv->LEN);
	add_send_flow(&csv->SRC_MAC, &csv->SRC_IP, csv->LEN);
	char output[100];
	strcpy(output, CSVFORMAT_to_str(csv));
	printf("%s\n", output);//输出至屏幕
	fprintf(fp, "%s\n", output);//输出至文件
}

//主函数
int main() {
	//打开文件
	fp = fopen(CSV_FILE_PATH, "w");

	//初始化流量统计
	//流量警告表
	if ((flow_alarm_list = (FLOW_STATEMENT_LIST*)malloc(sizeof(FLOW_STATEMENT_LIST))) == NULL)exit(-1);
	if ((flow_alarm_list->HEAD = (FLOW_STATEMENT*)malloc(sizeof(FLOW_STATEMENT))) == NULL)exit(-1);
	flow_alarm_list->length = 0;
	//流量发送表
	if ((flow_recv_list = (FLOW_STATEMENT_LIST*)malloc(sizeof(FLOW_STATEMENT_LIST))) == NULL)exit(-1);
	if ((flow_recv_list->HEAD = (FLOW_STATEMENT*)malloc(sizeof(FLOW_STATEMENT))) == NULL)exit(-1);
	flow_recv_list->length = 0;
	//流量接收表
	if ((flow_send_list = (FLOW_STATEMENT_LIST*)malloc(sizeof(FLOW_STATEMENT_LIST))) == NULL)exit(-1);
	if ((flow_send_list->HEAD = (FLOW_STATEMENT*)malloc(sizeof(FLOW_STATEMENT))) == NULL)exit(-1);
	flow_send_list->length = 0;

	//设备选择
	pcap_if_t* alldevs = getAllDevs();//获取设备列表
	int DevsCount = printAllDevs();//输出设备列表
	if (DevsCount == 0) {
		printf("\n未发现设备！请确认WinPcap已经安装！\n");
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
	last_sec = GetTickCount64();
	last_recv_output = GetTickCount64();
	last_send_output = GetTickCount64();

	//开始监听
	pcap_loop(handle, 0, packet_handler, NULL);

	return 0;
}