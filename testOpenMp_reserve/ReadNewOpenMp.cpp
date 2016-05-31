#include <iostream>
#include <sstream>
#include <fstream>
#include<time.h>
#include <bitset>
#include <omp.h>//尝试openMp来优化代码
using namespace std;
#define  ONE 1
#define ZERO 0
#define ROW 999
#define COLUMN 100000
#define BLANK 0xFFFFFFFF
static omp_lock_t lock;
int main()
{
	omp_init_lock(&lock);
	cout << "正在生成矩阵，请等待...\n";
	char filename[] = "E:\\GPU_Bitmap_Apriori\\data\\fimi\\h_T10I4D100K.txt";
	//char filename[] = "E:\\GPU_Bitmap_Apriori\\data\\fimi\\D110000.txt";
	FILE *fp = fopen(filename,"r");
	char strline[1024];
	int linenum = 0;//当前读取的第几行
	clock_t start_time = clock();
	//ifstream fin("E:\\GPU_Bitmap_Apriori\\data\\fimi\\h_T40I10D100K.txt");
	char **m = new char *[ROW];
	int row = 0,column = 0;
	#pragma omp parallel for
	for(int i = 0;i < ROW;i++)
	{
		m[i] = new char [COLUMN];
		memset(m[i],0,sizeof(m[i]));
	}
		#pragma omp parallel for private(strline) num_threads(6)
		for(int i = 0;i < COLUMN;i++)
		{
			string str;
			int curnum;
			#pragma omp critical (fp)
			//{
				//getline(fin,str);//一共COLUMN次	
			//}
			{
				fgets(strline,1024,fp);
				curnum = linenum++;
				str = strline;
			}
			//omp_unset_lock(&lock);
			int data = 0;
			istringstream istr(str);
			int index;
			bool flag = false;
			for (index =1;index <= ROW;index++)
			{
				if (!istr.eof() && flag == false )
				{
					istr >> data;
					flag = true;
				}
				if (data == index)
				{
					flag = false;
					m[index-1][curnum] = 1;
				}
				else
				{
					m[index-1][curnum] = 0;//下一个数字是同一个column不同的row
				}	
			}
		}
	//ofstream ofs("E:\\design\\read_reserve_T10I4D1K.bin", ios::binary);
	ofstream ofs("E:\\GPU_Bitmap_Apriori\\data\\fimi\\m_T10I4D100K_new.bin", ios::binary);

	int tranNum = COLUMN;
	int itemNum = 0;
	ofs.write((char*)&tranNum,sizeof(int));
	ofs.seekp(sizeof(int),ios::cur);//跳过4个字节用于存放项目数
	for(int i = 0;i < ROW;i++)
	{
		int itemId = i+1;
		//bool flag = false;//默认1-999项全部不输出
		ofs.write((char*)&itemId,sizeof(int));
		bitset<32> b(BLANK);
		ofs.write((char*)&b,sizeof(int));
		//输入项编号和FF FF FF FF共计8个字节
		for (int j = 0;j < COLUMN;)
		{
			char tmp = m[i][j]<<7|m[i][j+1]<<6|m[i][j+2]<<5|m[i][j+3]<<4|m[i][j+4]<<3|m[i][j+5]<<2|m[i][j+6]<<1|m[i][j+7];
			ofs.write(&tmp,1);
			j+=8;
		}
		//char tmp = 0;
		//ofs.write(&tmp,1);
		itemNum++;
	}
	//最后将itemNum放入BLANK所处的字节处
	ofs.seekp(4,ios::beg);
	ofs.write((char*)&itemNum,sizeof(int));
	ofs.close();
	for(int i = 0;i < ROW;i++)
	{
		delete [] m[i];
	}
	delete [] m;//注意要删除
	clock_t end_time=clock();
	omp_destroy_lock(&lock);
	cout <<"矩阵生成完成，请查看结果\n";
	cout<< "运行时间："<<static_cast<double>(end_time-start_time)/CLOCKS_PER_SEC*1000<<"ms"<<endl;
	cout << itemNum <<endl;

	getchar();
	return 0;
}
