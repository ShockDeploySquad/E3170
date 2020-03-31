#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "pcap.h"
#include <stdbool.h>
#include <time.h>

//������
#define FILTER "ip and udp"

//�������ޣ���λ B��
#define WARNING_LOWERLIMIT 1048576//1024*1024

//ͳ�����ݼ������λ ms��
#define STATEMENT_OUTPUT_INTERVAL 5000

//CSV�ĵ����Ŀ¼
#define CSV_FILE_PATH "log.csv"

//MAC��ַ
typedef struct MAC_ADDR {
	u_char byte0;
	u_char byte1;
	u_char byte2;
	u_char byte3;
	u_char byte4;
	u_char byte5;
}MAC_ADDR;

//IP��ַ
typedef struct IP_ADDR {
	u_char byte0;
	u_char byte1;
	u_char byte2;
	u_char byte3;
}IP_ADDR;

//�ж���MAC��ַ�Ƿ����
bool MAC_EQU(MAC_ADDR* MAC_ADDR_0, MAC_ADDR* MAC_ADDR_1) {
	return 
		(MAC_ADDR_0->byte0 == MAC_ADDR_1->byte0) &&
		(MAC_ADDR_0->byte1 == MAC_ADDR_1->byte1) &&
		(MAC_ADDR_0->byte2 == MAC_ADDR_1->byte2) &&
		(MAC_ADDR_0->byte3 == MAC_ADDR_1->byte3) &&
		(MAC_ADDR_0->byte4 == MAC_ADDR_1->byte4) &&
		(MAC_ADDR_0->byte5 == MAC_ADDR_1->byte5);
}

//�ж���IP��ַ�Ƿ����
bool IP_EQU(IP_ADDR* IP_ADDR_0, IP_ADDR* IP_ADDR_1) {
	return
		(IP_ADDR_0->byte0 == IP_ADDR_1->byte0) &&
		(IP_ADDR_0->byte1 == IP_ADDR_1->byte1) &&
		(IP_ADDR_0->byte2 == IP_ADDR_1->byte2) &&
		(IP_ADDR_0->byte3 == IP_ADDR_1->byte3);
}

//CSV��¼������
typedef struct CSV_FORMAT{
	struct tm* TIMESTAMP;	//ʱ���
	int TV_USEC;			//ʱ��������벿�֣�
	MAC_ADDR SRC_MAC;		//ԴMAC��ַ
	IP_ADDR SRC_IP;			//ԴIP��ַ
	MAC_ADDR DES_MAC;		//Ŀ��MAC��ַ
	IP_ADDR DES_IP;			//Ŀ��IP��ַ
	int LEN;				//֡����
}CSV_FORMAT;

//������¼���ĳһ��
typedef struct FLOW_STATEMENT {
	MAC_ADDR MAC;		//Դ/Ŀ��MAC��ַ
	IP_ADDR IP;			//Դ/Ŀ��IP��ַ
	unsigned TOL_FLOW;	//һ��ʱ���ڵ�������
}FLOW_STATEMENT;

//������¼��
typedef struct FLOW_STATEMENT_LIST {
	FLOW_STATEMENT* HEAD;//��ͷ
	int length;//��
}FLOW_STATEMENT_LIST;

//�豸�б�
pcap_if_t* all_devs;

//����ļ�
FILE* fp = NULL;

//������¼��
FLOW_STATEMENT_LIST* flow_alarm_list;
FLOW_STATEMENT_LIST* flow_recv_list;
FLOW_STATEMENT_LIST* flow_send_list;

//���������ʱtickcount
ULONGLONG last_sec;

//���豸MAC��ַ
MAC_ADDR self_mac_addr;

//ͳ�Ʊ������ʱ
ULONGLONG last_recv_output;
ULONGLONG last_send_output;

//��ȡ�豸�б������豸�б�
pcap_if_t* getAllDevs() {
	char error[PCAP_ERRBUF_SIZE];
	if (pcap_findalldevs(&all_devs, error) == -1){
		printf("Ѱ���豸ʱ��������: %s\n", error);
		exit(-1);
	}
	return all_devs;
}

//����豸�б������б��ȣ�
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

//ѡ���豸�������豸��Ϣ��
pcap_if_t* selectDev(int DevsCount) {
	int select_dev_index;
	printf("�������豸���(1-%d):", DevsCount);
	scanf("%d", &select_dev_index);
	if (select_dev_index <= 0 || select_dev_index > DevsCount){
		printf("�豸��Ӧ��(1-%d)�䣬ʵ������Ϊ%d�����볬�ޣ�\n", DevsCount, select_dev_index);
		pcap_freealldevs(all_devs);//�ͷ��豸�б�
		exit(-1);
	}
	pcap_if_t* current_dev;

	//��λ�����豸
	int temp_index = 0;
	for (current_dev = all_devs; temp_index < select_dev_index - 1; current_dev = current_dev->next, temp_index++);
	return current_dev;
}

//��ȡ���
pcap_t* getHandle(pcap_if_t* dev) {
	pcap_t* handle;
	char error[PCAP_ERRBUF_SIZE];
	
	//�򿪽ӿ�
	if ((handle = pcap_open_live(dev->name, 65536, 1, 1000, error)) == NULL) {
		printf("δ�ܴ򿪽ӿ���������WinPcap��֧��%s", dev->name);
		pcap_freealldevs(all_devs);
		exit(-1);
	}

	//����Ƿ�����̫��
	if (pcap_datalink(handle) != DLT_EN10MB){
		printf("�˳���ֻ����̫�������Ϲ�����\n");
		pcap_freealldevs(all_devs);
		exit(-1);
	}
	return handle;
}

//���ù�����
void setfilter(pcap_t* handle, u_int netmask) {
	struct bpf_program fcode;
	
	//����������ʽ
	if (pcap_compile(handle, &fcode, FILTER, 1, netmask) < 0) {
		printf("��������ʽ����\n");
		pcap_freealldevs(all_devs);
		exit(-1);
	}

	//���ù�����
	if (pcap_setfilter(handle, &fcode) < 0) {
		printf("���ù�����ʱ����\n");
		pcap_freealldevs(all_devs);
		exit(-1);
	}
}

//���յ���������ȡ�����������д��CSV������
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

//�������������д��CSV���ַ���
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

//����ͳ��
void add_alarm_flow(MAC_ADDR* mac_addr, IP_ADDR* ip_addr, int flow) {
	//��������
	if (GetTickCount64() - last_sec >= 1000) {
		for (int i = 0; i < flow_alarm_list->length; ++i) {
			(flow_alarm_list->HEAD + i)->TOL_FLOW = 0;
		}
		last_sec = GetTickCount64();
	}

	//����ͳ��
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

	//��������
	for (int i = 0; i < flow_alarm_list->length; ++i) {
		if ((flow_alarm_list->HEAD + i)->TOL_FLOW >= WARNING_LOWERLIMIT) {
			printf("����%02X-%02X-%02X-%02X-%02X-%02X,%3d:%3d:%3d:%3d�������������ƣ�\n",
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

//����ͳ��
void add_recv_flow(MAC_ADDR* mac_addr, IP_ADDR* ip_addr, int flow) {
	//����ͳ��
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

	//ͳ�����
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

//����ͳ��
void add_send_flow(MAC_ADDR* mac_addr, IP_ADDR* ip_addr, int flow) {
	//����ͳ��
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

	//ͳ�����
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

//�����¼�
void packet_handler(u_char* param, const struct pcap_pkthdr* header, const u_char* pkt_data){
	(VOID)(param);
	struct CSV_FORMAT* csv = pkt_to_csv(header, pkt_data);
	add_alarm_flow(&csv->SRC_MAC, &csv->SRC_IP, csv->LEN);
	add_recv_flow(&csv->DES_MAC, &csv->DES_IP, csv->LEN);
	add_send_flow(&csv->SRC_MAC, &csv->SRC_IP, csv->LEN);
	char output[100];
	strcpy(output, CSVFORMAT_to_str(csv));
	printf("%s\n", output);//�������Ļ
	fprintf(fp, "%s\n", output);//������ļ�
}

//������
int main() {
	//���ļ�
	fp = fopen(CSV_FILE_PATH, "w");

	//��ʼ������ͳ��
	//���������
	if ((flow_alarm_list = (FLOW_STATEMENT_LIST*)malloc(sizeof(FLOW_STATEMENT_LIST))) == NULL)exit(-1);
	if ((flow_alarm_list->HEAD = (FLOW_STATEMENT*)malloc(sizeof(FLOW_STATEMENT))) == NULL)exit(-1);
	flow_alarm_list->length = 0;
	//�������ͱ�
	if ((flow_recv_list = (FLOW_STATEMENT_LIST*)malloc(sizeof(FLOW_STATEMENT_LIST))) == NULL)exit(-1);
	if ((flow_recv_list->HEAD = (FLOW_STATEMENT*)malloc(sizeof(FLOW_STATEMENT))) == NULL)exit(-1);
	flow_recv_list->length = 0;
	//�������ձ�
	if ((flow_send_list = (FLOW_STATEMENT_LIST*)malloc(sizeof(FLOW_STATEMENT_LIST))) == NULL)exit(-1);
	if ((flow_send_list->HEAD = (FLOW_STATEMENT*)malloc(sizeof(FLOW_STATEMENT))) == NULL)exit(-1);
	flow_send_list->length = 0;

	//�豸ѡ��
	pcap_if_t* alldevs = getAllDevs();//��ȡ�豸�б�
	int DevsCount = printAllDevs();//����豸�б�
	if (DevsCount == 0) {
		printf("\nδ�����豸����ȷ��WinPcap�Ѿ���װ��\n");
		return -1;
	}
	pcap_if_t* current_dev = selectDev(DevsCount);//ѡ���豸

	//��ȡ���
	pcap_t* handle = getHandle(current_dev);

	//��������
	u_int netmask;
	if (current_dev->addresses != NULL)//��ǰ�豸��ַ��Ϊ����ȡ����
		netmask = ((struct sockaddr_in*)(current_dev->addresses->netmask))->sin_addr.S_un.S_addr;
	else netmask = 0xffffff;//�����豸��C����̫�������У�����Ϊ0xFFFFFF
	
	//������
	setfilter(handle, netmask);

	//����׼��
	printf("��ʼ����:%s\n", current_dev->description);
	pcap_freealldevs(alldevs);//�ͷ��豸�б�
	last_sec = GetTickCount64();
	last_recv_output = GetTickCount64();
	last_send_output = GetTickCount64();

	//��ʼ����
	pcap_loop(handle, 0, packet_handler, NULL);

	return 0;
}