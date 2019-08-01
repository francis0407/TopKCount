# TopK 实现思路

## 问题描述
有100GB的URL文件，限制1GB内存，求Top100的URL以及出现次数

## 问题分析

* 首先考虑Streaming的算法，因为这是一个可以使用伪流式算法（扫描多次）的场景。考虑MG算法，近似算法求topk，但是只能保证所有1/k的频繁度的对象能被找到，而题目中没有说url的分布情况，在极端情况下（例如有100个url出现两次，其他出现1次），使用MG算法不管扫描几次都不行。

* 其次考虑做分片再merge的方式，即首先做一次Hash分片，使得分片足够小，能放到内存里，再对各个分片做count，最后做topk merge。计算一下IO的次数，第一次做Hash分片一读一写2次IO，做count一读一写2次IO（一大一小），最后merge一次读，总共3 + 2次IO（三大两小）。

* 上述方式在第一次读之后做分片，实际可以考虑做一些优化的，比如我们直接做一个外存的链式Hash Map，一个slot对应一个文件，在读写的时候，做内存的Cache来减少IO，最后做merge。计算一下IO次数，读一次初始文件一次读，最后merge的时候一次读，只要保证中间建立Hash Map的过程不超过2 + 1次IO就理论上大概比上述算法有优势。考虑到，当url的分布越集中的时候，Cache miss理论上会越低。

## 解决方案

我们选择上述第三方案，做外存的链式Hash Map，通过内存的LRU Cache来减少IO，最后用一个Heap来做Merge。(还缺少更深入的思考，欢迎批评指正)

* 在外存HashMap中，我选择将Key(url) Value(count)分离，因为在这个场景下，Key是append only的，所以已经写好的block（或者page，我们暂用block表示）是只读的，在替换之后不需要写回，而Value的Block在修改之后一定是需要写回的，如果KV放在一起的话，会对URL做反复的读写。

* 在拿到一个url之后，我们先对他Hash，然后检查他在不在Cache过的Block里，如果在Cache过的Block里，可以直接避免IO，这对频繁项是应该是非常有效果的。如果不在Cache过的Block里，就检查其他Block，如果不存在于这个Hash slot里，那么就Append一个新的url。

* 对Key Block和Value Block分开做LRU Cache，主要原因是编码会方便一点。

* URLCache使用了640MB，CountCache用了128MB，剩下的用于一些meta data。

## 不足点

* 没有考虑url的性质，例如做url hash时可以选择特殊的hash function，其次是url的特殊结构可以做前缀压缩之类的(Trie树等)操作，目前仅把url当做简单的string处理。

* 没有做并发的处理，IO没有利用满，这样写起来大概会比较复杂。。

* 其实可以考虑在每个Hash Slot内部做一个数据结构的，例如二次Hash或者上一个BTree之类的，便于搜索Key，或者简单用一个BloomFilter也可以加速的。目前这种线性搜索的速度太慢了，随着BLock数量变多，后面的URL搜不动。
