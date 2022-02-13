#pragma once


template<class T,class F>
class tree_t {
	template<class T, class F>
	class item {
		item* previous;
		F assoc;
		item(item* prev, T set_mem, F set_association) : memory(set_mem), assoc(set_association) {
			previous = prev;
		}
		item(item* prev, item& it) : memory(it.memory), assoc(it.assoc) {
			previous = prev;
			if (it.right)
				add(*it.right);
			if (it.left)
				add(*it.left);
		}
	public:
		T memory;
		item* right = nullptr;
		item* left = nullptr;
		item(T set_mem, F set_association) : memory(set_mem), assoc(set_association) {
			previous = nullptr;
		}

		item(const item& copy) : memory(copy.memory), assoc(copy.assoc) {
			previous = nullptr;
			if (copy.right)
				right = new item(*copy.right);
			if (copy.left)
				left = new item(*copy.left);
		}

		item* add(item& it) {
			if (assoc > it.assoc) {
				if (!right) return right = new item(this, it);
				else return right->add(it);
			}
			else if (assoc == it.assoc) {
				return this;
			}
			else {
				if (!left) return left = new item(this, it);
				else return left->add(it);
			}
		}

		item const* get(const F& association) const {
			if (assoc == association)
				return this;
			if (assoc > association) {
				if (right) {
					const item* tmp = right->get(association);
					if (tmp)return tmp;
				}
				if (left) {
					const item* tmp = left->get(association);
					if (tmp)return tmp;
				}
			}
			else {
				if (left) {
					const item* tmp = left->get(association);
					if (tmp)return tmp;
				}
				if (right) {
					const item* tmp = right->get(association);
					if (tmp)return tmp;
				}
			}
			return nullptr;
		}
		item* get(const F& association) {
			if (assoc == association)
				return this;
			if (assoc > association) {
				if (right) {
					item* tmp = right->get(association);
					if (tmp)return tmp;
				}
				if (left) {
					item* tmp = left->get(association);
					if (tmp)return tmp;
				}
			}
			else {
				if (left) {
					item* tmp = left->get(association);
					if (tmp)return tmp;
				}
				if (right) {
					item* tmp = right->get(association);
					if (tmp)return tmp;
				}
			}
			return nullptr;
		}

		item* disconnect() {
			item* result = nullptr;
			if (right) {
				result = right;
				right->previous = nullptr;
				if (left)
					result->add(*left);
			}
			return result;
		}

		~item() {
			if (previous) {
				if (previous->right == this)
					previous->right = nullptr;
				else if (previous->left == this)
					previous->left = nullptr;
				if (right)
					previous->add(*right);
				if (left)
					previous->add(*left);
				return;
			}
			if (right)
				delete right;
			if (left)
				delete left;
		}

		static item* interate(bool& direction,item* it) {
			if (it) {
				if (direction) {
					direction = false;
					if (it->left) 
						return it->left;
					else if (it->right)
						return it->right;
				}
				else {
					if (it->right)
						return it->right;
					else if (it->left) {
						direction = true;
						return it->left;
					}
					else {
						if (it->previous)
							if (it->previous->previous) {
								direction = true;
								return it->previous->previous->left;
							}
					}
				}
			}
			return nullptr;
		}
	};
	item<T,F>* item_mem;
public:
	tree_t() : item_mem(nullptr){}
	tree_t(const tree_t& tree) {
		item_mem = new item<T, F>(*tree.item_mem);
	}

	item<T, F>* get(const F& asoc) {
		if (item_mem) {
			item<T, F>* elem = item_mem->get(asoc);
			if (elem)return elem;
			item<T, F> tmp(T(), asoc);
			return item_mem->add(tmp);
		}
		return item_mem = new item<T, F>(T(), asoc);
	}
	item<T, F> const* get(const F& asoc) const {
		return item_mem ? item_mem->get(asoc) : nullptr;
	}
	bool exist(const F& asoc) const {
		if (item_mem)
			return item_mem->get(asoc);
		else return 0;
	}
	void del(const F& asoc){
		if (item_mem) {
			if(
				item<T, F>* tmp = item_mem->get(asoc)
			  ) {
				if (tmp == item_mem) {
					item_mem = tmp->disconnect();
				}else delete tmp;
			}
		}
	}
	template<class FN_>
	void interate_all(FN_ fn) const {
		item<T, F>* iinterator = item_mem;
		bool direction = false;
		while (iinterator) {
			fn(iinterator->memory);
			iinterator = item<T, F>::interate(direction, iinterator);
		}
	}
	~tree_t() {
		if (item_mem)
			delete item_mem;
	}
};

template<class T>
class tree_self_asoc_t {
	template<class T>
	class item {
		item* previous;
		item(item* prev, T set_mem) : memory(set_mem) {
			previous = prev;
		}
	public:
		T memory;
		item* right = nullptr;
		item* left = nullptr;
		item(T set_mem) : memory(set_mem) {
			previous = nullptr;
		}

		item(const item& copy) : memory(copy.memory) {
			previous = nullptr;
			if (copy.right)
				right = new item(*copy.right);
			if (copy.left)
				left = new item(*copy.left);
		}

		bool exist(const T& association) {
			if (memory == association)
				return 1;
			else if (memory > association) {
				if (!right) return 0;
				else return right->exist(association);
			}
			else {
				if (!left) return 0;
				else return left->exist(association);
			}
		}
		item* get(const T& association) {
			if (memory == association)
				return this;
			if (memory > association) {
				if (right) {
					item* tmp = right->get(association);
					if (tmp)return tmp;
				}
				if (left) {
					item* tmp = left->get(association);
					if (tmp)return tmp;
				}
			}
			else {
				if (left) {
					item* tmp = left->get(association);
					if (tmp)return tmp;
				}
				if (right) {
					item* tmp = right->get(association);
					if (tmp)return tmp;
				}
			}
			return nullptr;
		}
		item* create(T asoc) {
			item tm(asoc);
			return create(tm);
		}
		item* create(item& it) {
			if (memory > it.memory) {
				if (!right) return right = new item(this, it.memory);
				else return right->create(it);
			}
			else if (memory == it.memory) {
				return this;
			}
			else {
				if (!left) return left = new item(this, it.memory);
				else return left->create(it);
			}
		}
		item* disconnect() {
			item* result = nullptr;
			if (right) {
				result = right;
				right->previous = nullptr;
				if (left)
					result->create(*left);
			}
			return result;
		}

		~item() {
			if (previous) {
				if (previous->right == this)
					previous->right = nullptr;
				else if (previous->left == this)
					previous->left = nullptr;
				if (right)
					previous->create(*right);
				if (left)
					previous->create(*left);
				return;
			}
			if (right)
				delete right;
			if (left)
				delete left;
		}
		static item* interate(bool& direction, item* it) {
			if (it) {
				if (direction) {
					direction = false;
					if (left)
						return left;
					else if (right)
						return right;
				}
				else {
					if (right)
						return right;
					else if (left) {
						direction = true;
						return left;
					}
					else {
						if (previous)
							if (previous->previous) {
								direction = true;
								return previous->previous->left;
							}
					}
				}
			}
			return nullptr;
		}
	};
	item<T>* item_mem;
public:
	tree_self_asoc_t() : item_mem(nullptr) {}
	tree_self_asoc_t(const tree_self_asoc_t& tree) {
		item_mem = new item<T>(*tree.item_mem);
	}

	bool exist(const T& asoc) {
		if (item_mem) {
			return item_mem->exist(asoc);
		}
		return 0;
	}
	void set(const T& asoc) {
		if (!item_mem) {
			item_mem = new item<T>(asoc);
		}
		else item_mem->create(asoc);
	}

	void del(const T& asoc) {
		if (item_mem) {
			if (
				item<T>* tmp = item_mem->get(asoc)
				) {
				if (tmp == item_mem) {
					item_mem = tmp->disconnect();
				}
				else delete tmp; 
			}
		}
	}
	template<class FN_>
	void interate_all(FN_ fn) const {
		item<T>* iinterator = item_mem;
		bool direction = false;
		while (iinterator) {
			fn(iinterator->memory);
			iinterator = item<T>::interate(direction, iinterator);
		}
	}
	~tree_self_asoc_t() {
		if (item_mem)
			delete item_mem;
	}
};
