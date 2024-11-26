#define _CRT_SECURE_NO_WARNINGS 1
#include "RBTree.h"
#include <string>
#include <utility>
#include <iostream>

namespace bit
{
    template<class K, class V>
    class map
    {
        struct MapKeyOfT
        {
            const K& operator()(const pair<const K, V>& kv) const
            {
                return kv.first;
            }
        };

    public:
        typedef typename RBTree<K, pair<const K, V>, MapKeyOfT>::Iterator iterator;

        iterator begin()
        {
            return _t.Begin();
        }
        iterator end()
        {
            return _t.End();
        }
        bool insert(const pair<K, V>& kv)
        {
            // ת��Ϊ pair<const K, V>
            pair<const K, V> new_kv(kv.first, kv.second);
            return _t.Insert(new_kv); // ����Ϊ���� new_kv
        }
    private:
        RBTree<K, pair<const K, V>, MapKeyOfT> _t;

    };

    void test_map()
    {
        bit::map<string, int> m;
        m.insert({ "xiang",1 });
        m.insert({ "df",2 });
        m.insert({ "rrr",4 });
        m.insert({ "x",5 });

        bit::map<string, int>::iterator it = m.begin();
        while (it != m.end())
        {
            // �޸�ֵ
            it->second += 1;

            // �����ֵ��
            cout << it->first << ":" << it->second << endl;
            ++it; // ����ǰ�õ���
        }
        cout << endl;
    }
}
