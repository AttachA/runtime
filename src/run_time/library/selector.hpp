// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <iterator>
#include <functional>
#include <list_array.hpp>


namespace art {
	namespace selector {
		template<class T>
		class Selector {
		public:
			virtual T& current() = 0;
			virtual const T& current() const = 0;
			virtual bool next() = 0;
		};
		template<class T>
		class SelectorInfo {
		protected:
			SelectorInfo<T>* prev;
		public:
			SelectorInfo(SelectorInfo<T>* prev):prev(prev) {}
			virtual ~SelectorInfo() {
				if(prev)
					delete prev;
			}
			virtual list_array<T*> selectedRefArr() = 0;
			virtual Selector<T>* getSelector() = 0;
			virtual SelectorInfo<T>* clone() = 0;
		};
		template<class T>
		class LASelectrorInfo : public SelectorInfo<T> {
			list_array<T> select;
		public:
			LASelectrorInfo(list_array<T>&& select) : select(select), SelectorInfo<T>(nullptr) {}
			LASelectrorInfo(const list_array<T>& select) : select(select), SelectorInfo<T>(nullptr) {}
			list_array<T*> selectedRefArr() override {
				list_array<T*> res(select.size());
				size_t i = 0;
				for (auto& it : select)
					res[i] = &select[i];
				return res;
			}
			Selector<T>* getSelector() override {
				class LocalSelector : public Selector<T> {
					list_array<T>::iterator<T> curr;
					list_array<T>::iterator<T> end;
					bool can_iter;
					bool iter_started = false;
				public:
					LocalSelector(list_array<T>& select) : curr(select.begin()), end(select.end()), can_iter(select.size()) {}

					T& current() override {
						return *curr;
					}
					const T& current() const override {
						return *curr;
					}
					bool next() override {
						if (can_iter) {
							if (!iter_started)
								return iter_started = true;
							if (curr == end)
								return false;
							curr++;
							return true;
						}
						return false;
					}
				};
				return new LocalSelector(select);
			}
			SelectorInfo<T>* clone() override {
				return new LASelectrorInfo<T>(select);
			};
		};

		template<class T>
		class RefsSelectrorInfo : public SelectorInfo<T> {
		protected:
			list_array<T*> select;
		public:
			RefsSelectrorInfo(const list_array<T*>& select, SelectorInfo<T>* prev) : select(select), SelectorInfo<T>(prev) {}
			RefsSelectrorInfo(const RefsSelectrorInfo& ) : select(select), SelectorInfo<T>(prev) {}
			RefsSelectrorInfo(list_array<T*>&& select) : select(select), SelectorInfo<T>(nullptr){}
			list_array<T*> selectedRefArr() override {return select;}
			Selector<T>* getSelector() override {
				
			}
			SelectorInfo<T>* clone() override {

				return new RefsSelectrorInfo<T>(select, SelectorInfo<T>::prev ? SelectorInfo<T>::prev->clone() : nullptr);
			};
		};

		template<class T>
		class WhereSelectrorInfo : public SelectorInfo<T> {
			std::function<bool(const T& it)> checker;
		public:
			WhereSelectrorInfo(SelectorInfo<T>* prev, std::function<bool(const T& it)> checker) : checker(checker), SelectorInfo<T>(prev) {}
			list_array<T*> selectedRefArr() override { return select; }
			Selector<T>* getSelector() override {
				class LocalSelector : public Selector<T> {
					list_array<T*>::iterator<T*> curr;
					list_array<T*>::iterator<T*> end;
					bool can_iter;
					bool iter_started = false;
				public:
					LocalSelector(list_array<T*>& select) : curr(select.begin()), end(select.end()), can_iter(select.size()) {}

					T& current() override {
						return **curr;
					}
					const T& current() const override {
						return **curr;
					}
					bool next() override {
						if (can_iter) {
							if (!iter_started)
								return iter_started = true;
							if (curr == end)
								return false;
							curr++;
							return true;
						}
						return false;
					}
				};
			}
			SelectorInfo<T>* clone() override {
				return new RefsSelectrorInfo<T>(select, prev ? prev->clone() : nullptr);
			};
		};

		template<class T>
		class JoinSelectorInfo : public RefsSelectrorInfo<T> {
			SelectorInfo<T>* joinable;
			JoinSelectorInfo(const list_array<T*>& select) : joinable(nullptr), RefsSelectrorInfo<T>(select, nullptr){}
		public:
			JoinSelectorInfo(SelectorInfo<T>* prev, SelectorInfo<T>* joinable) : joinable(joinable), RefsSelectrorInfo<T>(prev->selectedRefArr().join_copy(prev->selectedRefArr()),prev) {}
			template<class FN>
			JoinSelectorInfo(SelectorInfo<T>* prev, SelectorInfo<T>* joinable, FN join_selector) : joinable(joinable), RefsSelectrorInfo<T>(prev->selectedRefArr().join_copy(prev->selectedRefArr(), join_selector), prev) {}
			~JoinSelectorInfo() {
				if (joinable)
					delete joinable;
			}
		};

		template<class T>
		SelectorInfo<T>* Odd(SelectorInfo<T>* selector) {
			class OddSelectorInfo : public SelectorInfo<T>{
			public:
				OddSelectorInfo(SelectorInfo<T>* prev) :SelectorInfo<T>(prev){}
				list_array<T*> selectedRefArr() override {
					list_array<T*> res;
					Selector<T>* select = getSelector();
					while (select->next())
						res.push_back(&select->current());
					delete select;
					return res;
				}
				Selector<T>* getSelector() override {
					class OddSelector : public Selector<T> {
						Selector<T>* prev;
					public:
						OddSelector(Selector<T>* prev) :prev(prev) {}
						T& current() override {
							return prev->current();
						}
						const T& current() const override  {
							return prev->current();
						}
						bool next() override {
							return prev->next() ? prev->next() : false;
						}
					};
					return new OddSelector(prev->getSelector());
				}
			};
			return new OddSelectorInfo(selector);
		}
		template<class T>
		SelectorInfo<T>* Even(SelectorInfo<T>* selector) {
			class EvenSelectorInfo : public SelectorInfo<T> {
			public:
				EvenSelectorInfo(SelectorInfo<T>* prev) : SelectorInfo<T>(prev) {}
				list_array<T*> selectedRefArr() override {
					list_array<T*> res;
					Selector<T>* select = getSelector();
					while (select->next())
						res.push_back(&select->current());
					delete select;
					return res;
				}
				Selector<T>* getSelector() override {
					class EvenSelector : public Selector<T> {
						Selector<T>* prev;
					public:
						EvenSelector(Selector<T>* prev) :prev(prev) { prev->next(); }
						T& current() override {
							return prev->current();
						}
						const T& current() const override {
							return prev->current();
						}
						bool next() override {
							return prev->next() ? prev->next() : false;
						}
					} *res = new EvenSelector(prev->getSelector());
					return res;
				}
			}*res = new EvenSelectorInfo(selector);
			return res;
		}
		template<class T>
		SelectorInfo<T>* order_inc(SelectorInfo<T>* selector) {
			class OrderedSelectorInfo : public RefsSelectrorInfo<T> {
			public:
				OrderedSelectorInfo(SelectorInfo<T>* prev) : RefsSelectrorInfo(prev->selectedRefArr(),nullptr) { select.sort([](const T& l, const T& r) {return l < r; });}
			}*res = new OrderedSelectorInfo(selector);
			delete selector;
			return res;
		}
		template<class T>
		SelectorInfo<T>* order_dec(SelectorInfo<T>* selector) {
			class OrderedSelectorInfo : public RefsSelectrorInfo<T> {
			public:
				OrderedSelectorInfo(SelectorInfo<T>* prev) : RefsSelectrorInfo(prev->selectedRefArr(), nullptr) { select.sort([](const T& l, const T& r) {return l > r; }); }
			}*res = new OrderedSelectorInfo(selector);
			delete selector;
			return res;
		}

		template<class T,class _FN>
		SelectorInfo<T>* where(SelectorInfo<T>* selector, _FN function) {
			return new WhereSelectrorInfo(selector, function);
		}

		template<class T, class _FN>
		SelectorInfo<T>* join(SelectorInfo<T>* selector, SelectorInfo<T>* joinable, _FN function) {
			return new JoinSelectorInfo(selector, joinable, function);
		}

		template<class T>
		list_array<T> toLArr(SelectorInfo<T>* selector) {
			list_array<T> res(selector->selectedRefArr().convert([](T* it) { return *it; }));
			delete selector;
			return res;
		}
		template<class T>
		T* toArr(SelectorInfo<T>* selector) {
			list_array<T*> tmp = selector->selectedRefArr();
			T* res = new T[tmp.size()];
			size_t i = 0;
			for (T* it : tmp)
				res[i++] = *it;
			delete selector;
			return res;
		}


		template<class T,class FN>
		void iterate(SelectorInfo<T>* selector, FN iterator) {
			Selector* iter = selector->getSelector();
			while (iter->next()) {
				
			}
		}
	}
}