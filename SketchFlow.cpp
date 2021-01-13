//SketchFlow: Per-Flow Systematic Sampling Using Sketch Saturation Event
//p=-8 * ln(nOf0 / vector_size)
//2021-1-9
#include<fstream>
#include<iostream>
#include<string>
#include<ctime>
#include "crc32.hpp"
#include<set>
#include<map>

using namespace std;
typedef string Pkt; //��Ҫ��������ݰ���string��ʽ��

//��¼ÿ�����������˶��ٸ���
map<string, int> samplingCount;

class SF {
public:
	int L = 4;//layer. Sampling interval is 9.764^L.  -8*ln(0.25)=11.10
	float s = 0.7;//threshold

	uint32_t** W; // word array
	int word_n; // size of W
	int word_size = 32; // size of a word : b
	int vector_size = 8; // size of a virtual vector: b


	SF(int word_n,int L=4): word_n(word_n),L(L) {
		W = new uint32_t*[L];
		for(int i=0;i<L;i++)
			W[i] = new uint32_t[word_n]{};//��ʼ��Ϊ0
	}

	// when receive a Pkt, compute hash and virtual vector, update W, dicide sampling
	int recPkt(Pkt p) {
		uint32_t Hf = hash_crc(p); // hash val of a flow
		uint32_t index = Hf % word_size; //which backet
		set<int> indexOfmask; // �洢��ֵΪ0x1���Ƶ�λ��
		uint32_t virtual_vec = make_confined_vector(Hf, indexOfmask);

		for (int i = 0; i < L; i++) {
			W[i][index] |= leave_one_bit_only(virtual_vec, indexOfmask); //update counter

			if (popcount(W[i][index] & virtual_vec) >= vector_size*s) {
				W[i][index] &= (~virtual_vec);
				if (i == L - 1) sampling(p);
			}
			else
				break;
		}

		indexOfmask.clear();

		return 0;
	}

	void sampling(Pkt p) {
		samplingCount[p]++;//�ȵ���samplingCount[],�粻���ڣ����Զ�����0ֵ���ٶ�value����++
	}

	uint32_t hash_crc(Pkt p) {
		return Crc32(p.c_str(),p.size());
	}

	//����flowIDѡȡvector_sizeλ��Ϊ��������������
	//���ȸ���hash(flowIF)��32λ��������û������ڽ��г���(ÿ�λ�4λ��8�λ��ᣩ
	//���������Ľ���Ƿ���ȷֲ���û��֤��
	uint32_t make_confined_vector(uint32_t Hf, set<int> &indexOfmask) {
		int tmp = 0;
		for (int i = 0; i < 8; i++) {
			tmp = Hf % 32;
			Hf = Hf >> 4;
			if (!indexOfmask.count(tmp)) {
				indexOfmask.insert(tmp);
			}
		}
			
		for (int i = 1; i <= 8; i++) {
			tmp = Hf+i % 32;
			Hf = Hf >> 4;
			if (!indexOfmask.count(tmp)) {
				indexOfmask.insert(tmp);
			}
			if (indexOfmask.size() >= vector_size)
				break;
		}

		uint32_t res = 0;
		for (auto iter = indexOfmask.begin(); iter != indexOfmask.end(); iter++)
			res |= (0x1 << *iter);

		return res;
	}

	//���ѡȡ���������е�һλ
	uint32_t leave_one_bit_only(int vc, set<int> &indexOfmask) {
		int rnd = rand() % indexOfmask.size();
		set <int> ::const_iterator it(indexOfmask.begin());
		advance(it, rnd);

		return 0x1 << (*it);
	}

	//����v�Ķ����Ʊ�ʾ�С�1���ĸ���
	int popcount(uint32_t v) {
		int count = 0;

		for (count = 0; v; ++count)
			v &= (v - 1); // ������λ��1

		return count;
	}

	~SF() {
		for (int i = 0; i < L; i++)
			delete[] W[i];

		delete[]W;
	}

};

int main() {
	int word_n = 110 * 1024 / 4; // ���Ķ�CAIDA���Ĳ�����< 2^16
	SF sk(word_n,3);

	//srand((unsigned)time(NULL));

	ifstream inFile;
	inFile.open("D:\\program\\VisualStudio\\mySolution\\Debug\\20080415000.xu", ios::in);

	string eles[6]; //Oip, Dip, protocol, Oport, Dport, timestamp
	string strLine;
	Pkt p;
	int i = 1;
	while(getline(inFile, strLine)) {
		if (strLine.empty() || (!isalnum(strLine[0]))) continue; //��Ч���ݵĵ�һλӦΪ16�����ַ�

		//�⼸��������ͦ��ʱ�䣬����ռһ������
		//eles[0] = strLine.substr(0, 8);
		//eles[1] = strLine.substr(9, 8);
		//eles[2] = strLine.substr(18, 2);
		//eles[3] = strLine.substr(21, 4);
		//eles[4] = strLine.substr(26, 4);
		//eles[5] = strLine.substr(31);

		//p = eles[0] + eles[1] + eles[2] + eles[3] + eles[4];
		p = strLine.substr(0, 30);
		sk.recPkt(p);

		////��һ�β���������debug��
		//if (samplingCount.size() == 1) {
		//	printf("%d\n", i);
		//}

		if (i++ % 50000 == 0) printf("%d ����", (i-1)/10000);
	}
	inFile.close();

	ofstream outFile;
	outFile.open("D:\\program\\VisualStudio\\mySolution\\Debug\\res.txt");
	outFile<< "Oip+Dip+protocol+Oport+Dporte, count" << endl;
	long sum = 0;
	for (auto iter = samplingCount.begin(); iter != samplingCount.end(); iter++) {
		outFile << iter->first << ',' << iter->second << endl;
		sum += iter->second;
	}
	outFile << "sum: " << sum << endl;
	outFile.close();

	return 0;
}