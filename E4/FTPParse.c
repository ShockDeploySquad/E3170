#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "pcap.h"
#include <stdbool.h>
#include <time.h>
#include <string.h>

//�ַ�����ӦASCII��
#define U 0x55
#define S 0x53
#define E 0x45
#define R 0x52

#define P 0x50
#define A 0x41
//������
#define FILTER "tcp"

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

//����״̬
typedef enum ConnState {
	SEND_USER,				//�����û���
	RESP_USER,				//�ظ��û���
	SEND_PASS,				//��������
	RESP_PASS,				//�ظ�����
}ConnState;

//CSV��¼������
typedef struct CSV_FORMAT {
	struct tm* TIMESTAMP;	//ʱ���
	int TV_USEC;			//ʱ��������벿�֣�
	MAC_ADDR Client_MAC;	//�û�MAC��ַ
	IP_ADDR Client_IP;		//�û�IP��ַ
	MAC_ADDR FTP_MAC;		//FTP MAC��ַ
	IP_ADDR FTP_IP;			//FTP IP��ַ
	char* USER;				//�û�
	char* PASS;				//����
	boolean isSuccess;		//�����Ƿ�ɹ�
	char* msg;				//������Ϣ
}CSV_FORMAT;

//�豸�б�
pcap_if_t* all_devs;

//����״̬
ConnState state = SEND_USER;

//�����.csv�ļ���Ķ���
CSV_FORMAT csv;

//����ļ�
FILE* fp = NULL;

//��ȡ�豸�б������豸�б�
pcap_if_t* getAllDevs() {
	char error[PCAP_ERRBUF_SIZE];
	if (pcap_findalldevs(&all_devs, error) == -1) {
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
	if (select_dev_index <= 0 || select_dev_index > DevsCount) {
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
	if (pcap_datalink(handle) != DLT_EN10MB) {
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

//��֡�ж�ȡ�ַ�����������0x0D 0x0A��β��֡��
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

//MAC��ַת�ַ���
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

//IP��ַת�ַ���
char* IP_to_str(IP_ADDR* ip) {
	char* ip_str = (char*)malloc(16 * sizeof(char));
	sprintf(ip_str, "%3d:%3d:%3d:%3d",
		ip->byte0,
		ip->byte1,
		ip->byte2,
		ip->byte3);
	return ip_str;
}

//���ַ������߼�˫���ţ���Ӧcsv�����
char* add_quotes(char* str) {
	char* result = (char*)malloc((strlen(str) + 3) * sizeof(char));
	strcpy(&result[1], str);
	result[0] = '\"';
	result[strlen(str) + 1] = '\"';
	result[strlen(str) + 2] = '\0';
	return result;
}

//�������������д��CSV���ַ���
char* CSVFORMAT_to_str() {
	//���ֱ���ת�ַ���
	char* timestr = (char*)malloc(20 * sizeof(char));
	strftime(timestr, 20 * sizeof(char), "%Y/%m/%d %H:%M:%S", csv.TIMESTAMP);
	char* Client_MAC_str = MAC_to_str(&csv.Client_MAC);
	char* Client_IP_str = IP_to_str(&csv.Client_IP);
	char* FTP_MAC_str = MAC_to_str(&csv.FTP_MAC);
	char* FTP_IP_str = IP_to_str(&csv.FTP_IP);
	char* result = (char*)malloc(256 * sizeof(char));

	//���ת������ַ���
	sprintf(result, "%s:%06d,%s,%s,%s,%s,%s,%s,%s,%s\n",
		timestr,//ʱ��� 20�ֽ�
		csv.TV_USEC,//ʱ���:���� 6�ֽ�
		Client_MAC_str,//����MAC 18�ֽ�
		Client_IP_str,//����IP 16�ֽ�
		FTP_MAC_str,//FTPMAC 18�ֽ�
		FTP_IP_str,//FTPIP 16�ֽ�
		add_quotes(csv.USER),
		add_quotes(csv.PASS),
		csv.isSuccess ? "SUCCESS" : "FAILED",
		add_quotes(csv.msg));
	return result;
}

//���ļ���д����Ϣ
void write_str_to_file(char* str) {
	fp = fopen(CSV_FILE_PATH, "a");
	if (fp) {
		fprintf(fp, "%s", str);
		fclose(fp);
	}
	else printf("����ĵ���ռ�ã�д��ʧ��");
}

//���յ���������ȡ�����������д��CSV������
void pkt_to_csv(const struct pcap_pkthdr* header, const u_char* pkt_data) {
	time_t local_tv_sec = header->ts.tv_sec;
	u_char flag = pkt_data[0x2F];
	switch (state)
		//�����û���
		case SEND_USER: {
		csv.TIMESTAMP = localtime(&local_tv_sec);
		csv.TV_USEC = header->ts.tv_usec;
		if (flag == 0x18 &&//ȷ�ϱ�־λΪ0x18 [PSH, ACK] ��ͬ
			pkt_data[0x36] == U && pkt_data[0x37] == S &&
			pkt_data[0x38] == E && pkt_data[0x39] == R) {//0x36~0x39ΪUSER�ĸ��ַ�

			//��¼FTP���û���MAC��IP
			csv.Client_MAC = *(MAC_ADDR*)(&pkt_data[0x06]);
			csv.Client_IP = *(IP_ADDR*)(&pkt_data[0x1A]);
			csv.FTP_MAC = *(MAC_ADDR*)(&pkt_data[0x00]);
			csv.FTP_IP = *(IP_ADDR*)(&pkt_data[0x1E]);
			
			//��¼������û���
			csv.USER = read_msg(&pkt_data[0x3B]);
			printf("USERNAME:%s\n", csv.USER);

			//������ܷ���״̬
			state = RESP_USER;
		}
		break;

		//�����û��������Ƿ�ɹ�������Ϣ
		case RESP_USER: {
			if (flag == 0x18) {
				if (MAC_EQU(&csv.FTP_MAC, (MAC_ADDR*)(&pkt_data[0x06])) &&//ȷ��IP��MAC��ȷ
					IP_EQU(&csv.FTP_IP, (IP_ADDR*)(&pkt_data[0x1A])) &&
					MAC_EQU(&csv.Client_MAC, (MAC_ADDR*)(&pkt_data[0x00])) &&
					IP_EQU(&csv.Client_IP, (IP_ADDR*)(&pkt_data[0x1E]))) {
					if (pkt_data[0x36] == 0x33 && pkt_data[0x37] == 0x33 && pkt_data[0x38] == 0x31) {//��Ҫ����
						state = SEND_PASS;
					}
					else {//��������
						//�ص���ʼ״̬
						state == SEND_USER;

						//csv��д����Ϣ
						csv.msg = read_msg(&pkt_data[0x36]);

						//����csv
						write_str_to_file(CSVFORMAT_to_str());

						//���csv
						memset(&csv, 0, sizeof(CSV_FORMAT));
					}
					printf("%s\n", read_msg(&pkt_data[0x36]));
				}
			}
			break;
		}

		//��������
		case SEND_PASS: {
			if (flag == 0x18) {
				if (MAC_EQU(&csv.Client_MAC, (MAC_ADDR*)(pkt_data + 0x06)) &&
					IP_EQU(&csv.Client_IP, (IP_ADDR*)(pkt_data + 0x1A)) &&
					MAC_EQU(&csv.FTP_MAC, (MAC_ADDR*)(pkt_data)) &&
					IP_EQU(&csv.FTP_IP, (IP_ADDR*)(pkt_data + 0x1E)) &&
					pkt_data[0x36] == P && pkt_data[0x37] == A &&
					pkt_data[0x38] == S && pkt_data[0x39] == S) {
					//��¼���������
					csv.PASS = read_msg(&pkt_data[0x3B]);
					printf("PASSWORD:%s\n", csv.PASS);
				}
				state = RESP_PASS;
			}
			break;
		}
		
		//���������Ƿ���ȷ������Ϣ
		case RESP_PASS: {
			if (flag == 0x18) {
				if (MAC_EQU(&csv.FTP_MAC, (MAC_ADDR*)(pkt_data + 0x06)) &&
					IP_EQU(&csv.FTP_IP, (IP_ADDR*)(pkt_data + 0x1A)) &&
					MAC_EQU(&csv.Client_MAC, (MAC_ADDR*)(pkt_data)) &&
					IP_EQU(&csv.Client_IP, (IP_ADDR*)(pkt_data + 0x1E))) {
					if (pkt_data[0x36] == 0x35 && pkt_data[0x37] == 0x33 && pkt_data[0x38] == 0x30) {//530 ���벻��ȷ
						csv.isSuccess = false;
					}
					else if (pkt_data[0x36] == 0x32 && pkt_data[0x37] == 0x33 && pkt_data[0x38] == 0x30) {//230 ������ȷ
						csv.isSuccess = true;
					}
					csv.msg = read_msg(&pkt_data[0x36]);
					printf("%s\n\n", csv.msg);
					
					//����CSV
					write_str_to_file(CSVFORMAT_to_str());
				}
				state = SEND_USER;

				//���csv
				memset(&csv, 0, sizeof(CSV_FORMAT));
			}
			break;
		}
	}
}

//�����¼�
void packet_handler(u_char* param, const struct pcap_pkthdr* header, const u_char* pkt_data) {
	(VOID)(param);
	pkt_to_csv(header, pkt_data);
}

//������
int main() {
	//����ļ�
	fp = fopen(CSV_FILE_PATH, "w");
	if (fp) {
		fprintf(fp, "TimeStamp,Client MAC,Client IP,FTP MAC,FTP IP,Username,Password,Statement,Message\n");
		fclose(fp);
	}
	else {
		printf("����ĵ���ռ�ã��������");
		system("pause");
		return 0;
	}
	//�豸ѡ��
	pcap_if_t* alldevs = getAllDevs();//��ȡ�豸�б�
	int DevsCount = printAllDevs();//����豸�б�
	if (DevsCount == 0) {
		printf("\nδ�����豸����ȷ��WinPcap�Ѿ���װ��\n");
		system("pause");
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

	//��ʼ����
	pcap_loop(handle, 0, packet_handler, NULL);

	return 0;
}