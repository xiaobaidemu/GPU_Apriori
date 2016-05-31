# GPU_Apriori
testOpenMp和testOpenMp_reserve是用来将数据从水平格式转成二进制垂直格式，前者将自动去掉对应二进制全部为零的项，后者则保留全部项，选择后者，在GPU_Apriori中可以将对应项的支持度放入到FFFFFFFF中，方便后续计算,注意事务的大小必须为16的倍数，不足16倍数需要补充0。<br>
<br>
GPU_Apriori中需要输入二进制数据，并设定支持度值，二进制数据可以通过testOpenMp和testOpenMp_reserve得到，需要注意#define reserve，在运行程序前，需要将kernel文件下的dll文件放到C:\Windows\System32下<br>
<br>
GPUMiner_kernel是CUDA程序部分，生成了lib和上述的dll文件，使用低于CUDA5.0版本来进行编译，比较好。<br>
<br>
CPU_Apriori和GPU_Apriori类似，输入的数据位二进制数据，只是不使用CUDA对支持度进行计算。<br>
FUP_GPU和FUP_GPU_BIG是计算增量式的程序：<br>
<br>
FUP_GPU是计算候选项L(db)-L(DB)在数据集DB中中的支持度<br>
<br>
<br>
FUP_GPU_BIG是计算候选项L(DB)-L(db)在数据集db中中的支持度<br>
<br>
上述的两个程序，时间的计算上，不应该算上开始读取原频繁项集并对其进行处理的时间，建议不要使用Map来存储原频繁项集，后续的总时间不应该包括写出到文件的耗时。<br>
<br>
测试数据的使用以h_T10I4D1K,h_T10I4D10K,h_T10I4D100K,h_T4010D100K为基础使用转换程序生成二进制数据，其他可以使用的数据在data文件中<br>
