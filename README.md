# labs-Insight
Dive insight into CS-related labs for programming.Do, enjoy, and play. 

## CSAPP

[8] malloc
第8个实验是模拟glib上的内存分配，malloc， free, realloc三大函数
可以选择的数据结构有：
- implicit free list
- explicit free list
- segrated free list
- 其它

在作实验之前，我们需要知道glibc中malloc,free,realloc的工作原理。
我们说：
- malloc是内存分配管理 
- free是内存释放管理
- realloc是内存重新分配管理
注意，以上内存都是指的是动态内存，是在程序运行期间动态堆上进行分配和释放。



### 版本1，初入江湖

第一版本全部按照最原始的设想，数据结构采用implicit free lists，用一个全部变量指向list头(head_listp)，用于在malloc和realloc时查找相对应的block.

可以预见这种设计，每次malloc在查找一个空闲且容量足够的block，花费的时间将是O(n), n是list上所有的block的数量，包括分配的和空闲的。
这种方法的缺点性能差，实现简单，每次从头遍历。
结果分数也印证了上述分析，在速度方面只得了9分，可以说非常差劲了。
不过没有关系，这仅仅是第一个版本，我们还有很大的优化空间。

```shell
 * Results for mm malloc:
    trace  valid  util     ops      secs  Kops
    0       yes   99%    5694  0.011786   483
    1       yes   99%    5848  0.011177   523
    2       yes   99%    6648  0.016983   391
    3       yes  100%    5380  0.012709   423
    4       yes   66%   14400  0.000545 26437
    5       yes   92%    4800  0.014594   329
    6       yes   92%    4800  0.013741   349
    7       yes   55%   12000  0.190313    63
    8       yes   51%   24000  0.461914    52
    9       yes   27%   14401  0.142419   101
    10       yes   34%   14401  0.003235  4452
    Total          74%  112372  0.879417   128

    Perf index = 44 (util) + 9 (thru) = 53/100 
*/
```

### 版本2，小试牛刀

第一个版本的瓶颈在find_fit上，每次查找一个空闲且容量足够的block，我们需要遍历整个list。而版本1的一个设计缺陷在于所有allocated和free blocks全部在一个list上，虽然设计简单，但性能太差。

在版本2的设计上，我们沿用版本1的大多数设计风格，只是在find_fit上做优化。

其实，我们并不需要遍历allocated blocks，那么一个自然的想法就是设计两个list: 
- allocated list
- free list

每次malloc分配需要find_fit时，我们只需要遍历free list就行，这将会大大降低性能开销，尤其是已经存在大量allocated blocks的情况下。

```shell
    Results for mm malloc:
    trace  valid  util     ops      secs  Kops
    0       yes   86%    5694  0.000320 17777
    1       yes   90%    5848  0.000333 17546
    2       yes   94%    6648  0.000411 16191
    3       yes   95%    5380  0.000267 20180
    4       yes   66%   14400  0.000552 26068
    5       yes   84%    4800  0.001737  2763
    6       yes   82%    4800  0.001760  2727
    7       yes   55%   12000  0.000405 29644
    8       yes   51%   24000  0.000841 28537
    9       yes   26%   14401  0.147043    98
    10       yes   34%   14401  0.003212  4483
    Total          69%  112372  0.156882   716

    Perf index = 42 (util) + 40 (thru) = 82/100
```

实现结果分数表明，一个小小的数据结构设计优化就能大大提高算法的整体性能，得分从53分->82分，简直是火箭提速。


