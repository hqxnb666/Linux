#define _CRT_SECURE_NO_WARNINGS 1
#pragma once


//template <class Key, class Compare = less<Key>, class Alloc = alloc>
//class set {
//public:
//    typedef Key key_type;
//    typedef Key value_type;
//private:
//    typedef rb_tree<key_type, value_type,
//        identity<value_type>, key_compare, Alloc> rep_type;
//    rep_type t;  // red-black tree representing set
//}

// 抓重点
// 化繁为简，只关注跟抓重点部分有关系的，其他通通不关注

namespace bit
{
	template<class K>
	class set
	{
		struct SetKeyOfT
		{
			const K& operator()(const K& key)
			{
				return key;
			}
		};
	public:
		typedef typename RBTree<K, const K, SetKeyOfT>::Iterator iterator;

		iterator begin()
		{
			return _t.Begin();
		}

		iterator end()
		{
			return _t.End();
		}

		bool insert(const K& key)
		{
			return _t.Insert(key);
		}

	private:
		RBTree<K, const K, SetKeyOfT> _t;
	};

	void test_set()
	{
		set<int> s;
		s.insert(4);
		s.insert(2);
		s.insert(5);
		s.insert(15);
		s.insert(7);
		s.insert(1);
		s.insert(5);
		s.insert(7);

		set<int>::iterator it = s.begin();
		while (it != s.end())
		{
			//*it += 5;

			cout << *it << " ";
			++it;
		}
		cout << endl;

		for (auto e : s)
		{
			cout << e << endl;
		}
	}
}