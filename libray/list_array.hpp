#ifndef LIST_ARRAY
#define LIST_ARRAY
#include <stdint.h>
#include <utility>
#include <type_traits>
#include <stdexcept>
#include <iterator>

#if __cplusplus  >= 202002L
#define req(require) requires require
#define conexpr constexpr
#else
#define req(require)
#define conexpr
#endif

template<class T>
class list_array {
	template<class T>
	class dynamic_arr;
	template<class T>
	class arr_block;
	template<class T>
	conexpr static T* cxxresize(T* val, size_t old_size, size_t new_size) {
		if (new_size == old_size)
			return val;
		T* new_val = new T[new_size];
		if (old_size < new_size) {
			if constexpr (std::is_move_assignable<T>::value)
				for (size_t i = 0; i < old_size; i++)
					new_val[i] = std::move(val[i]);
			else
				for (size_t i = 0; i < old_size; i++)
					new_val[i] = val[i];
		}
		else {
			if constexpr (std::is_move_assignable<T>::value)
				for (size_t i = 0; i < new_size; i++)
					new_val[i] = std::move(val[i]);
			else
				for (size_t i = 0; i < new_size; i++)
					new_val[i] = val[i];
		}
		delete[] val;
		return new_val;
	}
public:
	using value_type = T;
	using reference = T&;
	using const_reference = const T&;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	static constexpr inline const size_t npos = -1;

	template<class T>
	class reverse_iterator;
	template<class T>
	class const_iterator;
	template<class T>
	class const_reverse_iterator;
	template<class T = T>
	class iterator {
		friend class list_array<T>;
		friend class dynamic_arr<T>;
		friend class const_iterator<T>;
		friend class reverse_iterator<T>;
		friend class const_reverse_iterator<T>;
		arr_block<T>* block;
		size_t pos;
		conexpr bool _nextBlock() {
			block = block ? block->next_ : block;
			pos = 0;
			return block && (block ? block->next_ : block);
		}
		conexpr void _fast_load(T* arr, size_t arr_size) {
			size_t j = pos;
			arr_block<T>* block_tmp = block;
			size_t block_size = block_tmp->_size;
			T* block_arr = block->arr_contain;

			for (size_t i = 0; i < arr_size;) {
				for (; i < arr_size && j < block_size; j++)
					arr[i++] = block_arr[j];
				j = 0;
				block_tmp = block_tmp->next_;
				if (!block_tmp)return;
				block_size = block_tmp->_size;
				block_arr = block_tmp->arr_contain;
			}
		}
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = T;
		using difference_type = ptrdiff_t;
		using pointer = T*;
		using reference = T&;

		conexpr iterator() { block = nullptr; pos = 0; }
		conexpr iterator& operator=(const iterator& seter) {
			block = seter.block;
			pos = seter.pos;
			return *this;
		}
		conexpr iterator(const iterator& copy) {
			*this = copy;
		}
		conexpr iterator(arr_block<T>* block_pos, size_t set_pos) {
			block = block_pos;
			pos = set_pos;
			if (block)
				if (block->_size == set_pos && block->next_)
				{
					block = block->next_;
					pos = 0;
				}
		}
		conexpr iterator& operator++() {
			if (block) {
				if (block->_size <= ++pos) {
					block = block->next_;
					pos = 0;
				}
			}
			return *this;
		}
		conexpr iterator operator++(int) {
			iterator tmp = *this;
			operator++();
			return tmp;
		}
		conexpr iterator& operator--() {
			if (block) {
				if (0 == --pos) {
					block = block->_prev;
					pos = block ? block->_size : 0;
				}
			}
			return *this;
		}
		conexpr iterator operator--(int) {
			iterator tmp = *this;
			operator--();
			return tmp;
		}
		conexpr bool operator==(const iterator& comparer) const {
			return block == comparer.block && pos == comparer.pos && block;
		}
		conexpr bool operator!=(const iterator& comparer) const {
			return (block != comparer.block || pos != comparer.pos) && block;
		}
		conexpr T& operator*() { return block->arr_contain[pos]; }
		conexpr const T& operator*() const { return block->arr_contain[pos]; }
		conexpr T* operator->() { return block->arr_contain + pos; }
	};
	template<class T = T>
	class const_iterator {
		friend class list_array<T>;
		friend class dynamic_arr<T>;
		friend class const_reverse_iterator<T>;
		const arr_block<T>* block;
		size_t pos;
		conexpr void _fast_load(T* arr, size_t arr_size) const {
			size_t j = pos;
			const arr_block<T>* block_tmp = block;
			size_t block_size = block_tmp->_size;
			T* block_arr = block->arr_contain;

			for (size_t i = 0; i < arr_size;) {
				for (; i < arr_size && j < block_size; j++)
					arr[i++] = block_arr[j];
				j = 0;
				block_tmp = block_tmp->next_;
				if (!block_tmp)return;
				block_size = block_tmp->_size;
				block_arr = block_tmp->arr_contain;
			}
		}
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = T;
		using difference_type = ptrdiff_t;
		using pointer = T*;
		using reference = T&;

		conexpr const_iterator() { block = nullptr; pos = 0; }
		conexpr const_iterator& operator=(const const_iterator& seter) {
			block = seter.block;
			pos = seter.pos;
			return *this;
		}
		conexpr const_iterator(const const_iterator& copy) {
			*this = copy;
		}
		conexpr const_iterator& operator=(const iterator<T>& seter) {
			block = seter.block;
			pos = seter.pos;
			return *this;
		}
		conexpr const_iterator(const iterator<T>& copy) {
			*this = copy;
		}
		conexpr const_iterator(arr_block<T>* block_pos, size_t set_pos) {
			block = block_pos;
			pos = set_pos;
		}
		conexpr const_iterator& operator++() {
			if (block) {
				if (block->_size <= ++pos) {
					block = block->next_;
					pos = 0;
				}
			}
			return *this;
		}
		conexpr const_iterator operator++(int) {
			const_iterator tmp = *this;
			operator++();
			return tmp;
		}
		conexpr const_iterator& operator--() {
			if (block) {
				if (0 == --pos) {
					block = block->_prev;
					pos = block ? block->_size : 0;
				}
			}
			return *this;
		}
		conexpr const_iterator operator--(int) {
			const_iterator tmp = *this;
			operator--();
			return tmp;
		}
		conexpr bool operator==(const const_iterator& comparer) const {
			return block == comparer.block && pos == comparer.pos;
		}
		conexpr bool operator!=(const const_iterator& comparer) const {
			return (block != comparer.block || pos != comparer.pos) && block;
		}
		conexpr const T& operator*() {
			return block->arr_contain[pos];
		}
		conexpr T* operator->() { return block->arr_contain + pos; }
	};
	template<class T = T>
	class reverse_iterator {
		friend class dynamic_arr<T>;
		friend class const_reverse_iterator<T>;
		arr_block<T>* block;
		size_t pos;
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = T;
		using difference_type = ptrdiff_t;
		using pointer = T*;
		using reference = T&;

		conexpr reverse_iterator() { block = nullptr; pos = 0; }
		conexpr reverse_iterator& operator=(const iterator<T>& seter) {
			block = seter.block;
			pos = seter.pos;
			return *this;
		}
		conexpr reverse_iterator(const iterator<T>& copy) {
			*this = copy;
		}
		conexpr reverse_iterator& operator=(const reverse_iterator& seter) {
			block = seter.block;
			pos = seter.pos;
			return *this;
		}
		conexpr reverse_iterator(const reverse_iterator& copy) {
			*this = copy;
		}
		conexpr reverse_iterator(arr_block<T>* block_pos, size_t set_pos) {
			block = block_pos;
			pos = set_pos;
		}
		conexpr reverse_iterator& operator++() {
			if (block) {
				if (0 == --pos) {
					block = block->_prev;
					pos = block ? block->_size : 0;
				}
			}
			return *this;
		}
		conexpr reverse_iterator operator++(int) {
			reverse_iterator tmp = *this;
			operator++();
			return tmp;
		}
		conexpr reverse_iterator& operator--() {
			if (block) {
				if (block->_size == ++pos) {
					block = block->next_;
					pos = 0;
				}
			}
			return *this;
		}
		conexpr reverse_iterator operator--(int) {
			reverse_iterator tmp = *this;
			operator--();
			return tmp;
		}
		conexpr bool operator==(const reverse_iterator& comparer) const {
			return block == comparer.block && pos == comparer.pos;
		}
		conexpr bool operator!=(const reverse_iterator& comparer) const {
			return (block != comparer.block || pos != comparer.pos) && block;
		}
		conexpr T& operator*() {
			return block->arr_contain[pos - 1];
		}
		conexpr const T& operator*() const {
			return block->arr_contain[pos - 1];
		}
		conexpr T* operator->() {
			return block->arr_contain + pos - 1;
		}
	};
	template<class T = T>
	class const_reverse_iterator {
		friend class dynamic_arr<T>;
		friend class const_iterator<T>;
		const arr_block<T>* block;
		size_t pos;
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = T;
		using difference_type = ptrdiff_t;
		using pointer = T*;
		using reference = T&;

		conexpr const_reverse_iterator() { block = nullptr; pos = 0; }
		conexpr const_reverse_iterator& operator=(const const_iterator<T>& seter) {
			block = seter.block;
			pos = seter.pos;
			return *this;
		}
		conexpr const_reverse_iterator(const const_iterator<T>& copy) {
			*this = copy;
		}
		conexpr const_reverse_iterator& operator=(const iterator<T>& seter) {
			block = seter.block;
			pos = seter.pos;
			return *this;
		}
		conexpr const_reverse_iterator(const iterator<T>& copy) {
			*this = copy;
		}
		conexpr const_reverse_iterator& operator=(const reverse_iterator<T>& seter) {
			block = seter.block;
			pos = seter.pos;
			return *this;
		}
		conexpr const_reverse_iterator(const reverse_iterator<T>& copy) {
			*this = copy;
		}
		conexpr const_reverse_iterator& operator=(const const_reverse_iterator& seter) {
			block = seter.block;
			pos = seter.pos;
			return *this;
		}
		conexpr const_reverse_iterator(const const_reverse_iterator& copy) {
			*this = copy;
		}
		conexpr const_reverse_iterator(arr_block<T>* block_pos, size_t set_pos) {
			block = block_pos;
			pos = set_pos;
		}
		conexpr const_reverse_iterator& operator++() {
			if (block) {
				if (0 == --pos) {
					block = block->_prev;
					pos = block ? block->_size : 0;
				}
			}
			return *this;
		}
		conexpr const_reverse_iterator operator++(int) {
			const_reverse_iterator tmp = *this;
			operator++();
			return tmp;
		}
		conexpr const_reverse_iterator& operator--() {
			if (block) {
				if (block->_size == ++pos) {
					block = block->next_;
					pos = 0;
				}
			}
			return *this;
		}
		conexpr const_reverse_iterator operator--(int) {
			const_reverse_iterator tmp = *this;
			operator--();
			return tmp;
		}
		conexpr bool operator==(const const_reverse_iterator& comparer) const {
			return block == comparer.block && pos == comparer.pos;
		}
		conexpr bool operator!=(const const_reverse_iterator& comparer) const {
			return (block != comparer.block || pos != comparer.pos) && block;
		}
		conexpr const T& operator*() const {
			return block->arr_contain[pos - 1];
		}
		conexpr T* operator->() {
			return block->arr_contain + pos - 1;
		}
	};

	class range_provider {
		size_t _start;
		size_t _end;
		list_array<T>& ln;
	public:
		conexpr range_provider(list_array<T>& link, size_t start, size_t end) : ln(link) { _start = start; _end = end; }
		conexpr range_provider(const range_provider& copy) : ln(copy.ln) { _start = copy._start; _end = copy._end; }
		conexpr iterator<T> begin() {
			return ln.get_iterator(_start);
		}
		conexpr iterator<T> end() {
			return ln.get_iterator(_end);
		}
		conexpr const_iterator<T> begin() const {
			return ln.get_iterator(_start);
		}
		conexpr const_iterator<T> end() const {
			return ln.get_iterator(_end);
		}
		conexpr reverse_iterator<T> rbegin() {
			return ln.get_iterator(_end);
		}
		conexpr reverse_iterator<T> rend() {
			return ln.get_iterator(_start);
		}
		conexpr const_reverse_iterator<T> rbegin() const {
			return ln.get_iterator(_end);
		}
		conexpr const_reverse_iterator<T> rend() const {
			return ln.get_iterator(_start);
		}
		conexpr size_t range_start() const {
			return _start;
		}
		conexpr size_t range_end() const {
			return _end;
		}
	};
	class const_range_provider {
		size_t _start;
		size_t _end;
		const list_array<T>& ln;
	public:
		conexpr const_range_provider(const list_array<T>& link, size_t start, size_t end) : ln(link) { _start = start; _end = end; }
		conexpr const_range_provider(const const_range_provider& copy) : ln(copy.ln) { _start = copy._start; _end = copy._end; }
		conexpr const_iterator<T> begin() const {
			return ln.get_iterator(_start);
		}
		conexpr const_iterator<T> end() const {
			return ln.get_iterator(_end);
		}
		conexpr const_reverse_iterator<T> rbegin() const {
			return ln.get_iterator(_end);
		}
		conexpr const_reverse_iterator<T> rend() const {
			return ln.get_iterator(_start);
		}
	};
	class reverse_provider {
		reverse_iterator<T> _begin;
		reverse_iterator<T> _end;
	public:
		conexpr reverse_provider(range_provider& link) { _begin = link.rbegin(); _end = link.rend(); }
		conexpr reverse_provider(range_provider&& link) { _begin = link.rbegin(); _end = link.rend(); }
		conexpr reverse_provider(list_array<T>& link) { _begin = link.rbegin(); _end = link.rend(); }
		conexpr reverse_provider(const reverse_provider& copy) { _begin = copy._begin; _end = copy._end; }
		conexpr reverse_iterator<T> begin() {
			return _begin;
		}
		conexpr reverse_iterator<T> end() {
			return _end;
		}
		conexpr const_reverse_iterator<T> begin() const {
			return _begin;
		}
		conexpr const_reverse_iterator<T> end() const {
			return _end;
		}
	};
	class const_reverse_provider {
		const_reverse_iterator<T> _begin;
		const_reverse_iterator<T> _end;
	public:
		conexpr const_reverse_provider(const range_provider& link) { _begin = link.rbegin(); _end = link.rend(); }
		conexpr const_reverse_provider(const range_provider&& link) { _begin = link.rbegin(); _end = link.rend(); }
		conexpr const_reverse_provider(const const_range_provider& link) { _begin = link.rbegin(); _end = link.rend(); }
		conexpr const_reverse_provider(const const_range_provider&& link) { _begin = link.rbegin(); _end = link.rend(); }
		conexpr const_reverse_provider(const list_array<T>& link) { _begin = link.rbegin(); _end = link.rend(); }
		conexpr const_reverse_provider(const const_reverse_provider& copy) { _begin = copy._begin; _end = copy._end; }
		conexpr const_reverse_iterator<T> begin() const {
			return _begin;
		}
		conexpr const_reverse_iterator<T> end() const {
			return _end;
		}
	};
private:
	//block item abstraction
	template<class T>
	class arr_block {
		friend class list_array<T>;
		friend class dynamic_arr<T>;
		arr_block* _prev = nullptr;
		arr_block* next_ = nullptr;
		T* arr_contain = nullptr;
		size_t _size = 0;
		void good_bye_world() {
			if (_prev)
				_prev->next_ = next_;
			if (next_)
				next_->_prev = _prev;
			_prev = next_ = nullptr;
			delete this;
		}
	public:
		conexpr arr_block() {}
		conexpr arr_block(const arr_block& copy) { operator=(copy); }
		conexpr arr_block(arr_block&& move) noexcept { operator=(std::move(move)); }
		conexpr arr_block(arr_block* prev, size_t len, arr_block* next) {
			if (_prev = prev)
				_prev->next_ = this;
			if (next_ = next)
				next_->_prev = this;
			arr_contain = new T[len];
			_size = len;
		}
		conexpr ~arr_block() {
			if (_prev) {
				_prev->next_ = nullptr;
				delete _prev;
			}
			else if (next_) {
				next_->_prev = nullptr;
				delete next_;
			}
			if (arr_contain)
				delete[] arr_contain;
		}
		conexpr T& operator[](size_t pos) {
			return (pos < _size) ? arr_contain[pos] : (*next_)[pos - _size];
		}
		conexpr const T& operator[](size_t pos) const {
			return (pos < _size) ? arr_contain[pos] : (*next_)[pos - _size];
		}
		conexpr arr_block& operator=(const arr_block& copy) {
			_size = copy._size;
			arr_contain = new T[_size];
			for (size_t i = 0; i < _size; i++)
				arr_contain[i] = copy.arr_contain[i];
		}
		conexpr arr_block& operator=(arr_block&& move) noexcept {
			arr_contain = move.arr_contain;
			_prev = move._prev;
			next_ = move.next_;
			_size = move._size;
			move._prev = move.next_ = nullptr;
			move.arr_contain = nullptr;
		}
		conexpr T& index_back(size_t pos) {
			return (pos < _size) ? arr_contain[_size - pos - 1] : (*_prev).index_back(pos - _size);
		}
		conexpr T& index_front(size_t pos) {
			return (pos < _size) ? arr_contain[pos] : (*next_)[pos - _size];
		}
		conexpr const T& index_back(size_t pos) const {
			return (pos < _size) ? arr_contain[_size - pos - 1] : (*_prev).index_back(pos - _size);
		}
		conexpr const T& index_front(size_t pos) const {
			return (pos < _size) ? arr_contain[pos] : (*next_)[pos - _size];
		}
		conexpr iterator<T> get_iterator(size_t pos) {
			if (!this) return iterator<T>(nullptr, pos);
			return
				this ?
				((pos < _size) ?
					iterator<T>(this, pos) :
					(*next_).get_iterator(pos - _size)
					) :
				iterator<T>(nullptr, 0);
		}
		conexpr iterator<T> get_iterator_back(size_t pos) {
			if (!this) return iterator<T>(nullptr, 0);
			return
				this ?
				((pos < _size) ?
					iterator<T>(this, _size - pos - 1) :
					(*_prev).get_iterator_back(pos - _size)
					) :
				iterator<T>(nullptr, 0);
		}
		conexpr const const_iterator<T> get_iterator(size_t pos) const {
			if (!this) return const_iterator<T>(nullptr, pos);
			return
				this ?
				((pos < _size) ?
					const_iterator<T>(this, pos) :
					(*next_).get_iterator(pos - _size)
					) :
				const_iterator<T>(nullptr, 0);
		}
		conexpr const const_iterator<T> get_iterator_back(size_t pos) const {
			if (!this) return const_iterator<T>(nullptr, 0);
			return
				this ?
				((pos < _size) ?
					const_iterator<T>(this, _size - pos - 1) :
					(*_prev).get_iterator_back(pos - _size)
					) :
				const_iterator<T>(nullptr, 0);
		}
		conexpr iterator<T> begin() {
			return
				this ? (
					_prev ?
					_prev->begin() :
					iterator<T>(this, 0)
					) :
				iterator<T>(nullptr, 0)
				;
		}
		conexpr iterator<T> end() {
			return
				this ? (
					next_ ?
					next_->end() :
					iterator<T>(this, _size)
					) :
				iterator<T>(nullptr, 0)
				;
		}
		conexpr const_iterator<T> begin() const {
			return
				this ? (
					_prev ?
					_prev->begin() :
					const_iterator<T>(this, 0)
					) :
				const_iterator<T>(nullptr, 0)
				;
		}
		conexpr const_iterator<T> end() const {
			return
				this ? (
					next_ ?
					next_->end() :
					iterator<T>(this, _size)
					) :
				iterator<T>(nullptr, 0)
				;
		}
		conexpr inline size_t size() const {
			return _size;
		}
		conexpr void resize_front(size_t siz) {
			if (!siz) {
				good_bye_world();
				return;
			}
			T* tmp = arr_contain;
			arr_contain = cxxresize<T>(arr_contain, _size, siz);
			if (arr_contain == nullptr)
			{
				arr_contain = tmp;
				throw std::bad_alloc();
			}
			_size = siz;
		}
		conexpr void resize_begin(size_t siz) {
			if (!siz) {
				good_bye_world();
				return;
			}
			T* narr = new T[siz];
			if (narr == nullptr)
				throw std::bad_alloc();

			int64_t dif = _size - siz;
			if (dif > 0)
				for (size_t i = 0; i < siz && i < _size; i++)
					narr[i] = arr_contain[i + dif];
			else {
				dif *= -1;
				for (size_t i = 0; i < siz && i < _size; i++)
					narr[dif + i] = arr_contain[i];
			}
			delete[] arr_contain;
			arr_contain = narr;
			_size = siz;
		}
	};
	//list of arrays abstraction
	template<class T>
	class dynamic_arr {
		friend class list_array<T>;
		arr_block<T>* arr = nullptr;
		arr_block<T>* arr_end = nullptr;
		size_t _size = 0;

		conexpr void swap_block_with_blocks(arr_block<T>& this_block, arr_block<T>& first_block, arr_block<T>& second_block) {
			if (this_block._prev)
				this_block._prev->next_ = &first_block;
			if (this_block.next_)
				this_block.next_->_prev = &second_block;

			if (arr == &this_block)
				arr = &first_block;
			if (arr_end == &this_block)
				arr_end = &second_block;

			this_block._prev = this_block.next_ = nullptr;
			this_block.good_bye_world();
		}


		conexpr void remove_item_slow(size_t pos, arr_block<T>& this_block) {
			if (pos > this_block._size / 2) {
				size_t mov_to = this_block._size - 1;
				for (size_t i = pos; i < mov_to; i++)
					std::swap(this_block.arr_contain[i + 1], this_block.arr_contain[i]);
				this_block.resize_front(this_block._size - 1);
			}
			else
			{
				for (int64_t i = pos; i > 0; i--)
					std::swap(this_block.arr_contain[i - 1], this_block.arr_contain[i]);
				this_block.resize_begin(this_block._size - 1);
			}
		}
		conexpr void remove_item_split(size_t pos, arr_block<T>& this_block) {
			size_t block_size = this_block._size;
			arr_block<T>& first_block = *new arr_block<T>(nullptr, pos, nullptr);
			arr_block<T>& second_block = *new arr_block<T>(&first_block, block_size - pos - 1, nullptr);

			for (size_t i = 0; i < pos; i++)
				first_block.arr_contain[i] = this_block.arr_contain[i];

			size_t block_half_size = pos + 1;
			for (size_t i = block_half_size; i < block_size; i++)
				second_block.arr_contain[i - block_half_size] = this_block.arr_contain[i];

			swap_block_with_blocks(this_block, first_block, second_block);
		}

		conexpr void insert_item_slow(size_t pos, arr_block<T>& this_block, const T& item) {
			if (pos > this_block._size / 2) {
				this_block.resize_front(this_block._size + 1);
				size_t mov_to = this_block._size - 1;
				for (int64_t i = mov_to - 1; i >= pos; i--)
					std::swap(this_block.arr_contain[i + 1], this_block.arr_contain[i]);
				this_block.arr_contain[pos] = item;
			}
			else
			{
				this_block.resize_begin(this_block._size + 1);
				for (int64_t i = 0; i < pos; i++)
					std::swap(this_block.arr_contain[i + 1], this_block.arr_contain[i]);
				this_block.arr_contain[pos] = item;
			}
		}
		conexpr void insert_item_split(size_t pos, arr_block<T>& this_block, const T& item) {
			size_t block_size = this_block._size;
			size_t first_size = pos + 1;
			arr_block<T>& first_block = *new arr_block<T>(nullptr, first_size + 1, nullptr);
			arr_block<T>& second_block = *new arr_block<T>(&first_block, block_size - first_size, nullptr);

			for (size_t i = 0; i < pos; i++)
				first_block.arr_contain[i] = this_block.arr_contain[i];
			first_block[first_size] = item;
			for (size_t i = first_size; i < block_size; i++)
				second_block.arr_contain[i - first_size] = this_block.arr_contain[i];
			swap_block_with_blocks(this_block, first_block, second_block);
		}
		conexpr void insert_item_slow(size_t pos, arr_block<T>& this_block, T&& item) {
			if (pos > this_block._size / 2) {
				this_block.resize_front(this_block._size + 1);
				size_t mov_to = this_block._size - 1;
				for (int64_t i = mov_to - 1; i >= pos; i--)
					std::swap(this_block.arr_contain[i + 1], this_block.arr_contain[i]);
				this_block.arr_contain[pos] = std::move(item);
			}
			else
			{
				this_block.resize_begin(this_block._size + 1);
				for (int64_t i = 0; i < pos; i++)
					std::swap(this_block.arr_contain[i + 1], this_block.arr_contain[i]);
				this_block.arr_contain[pos] = std::move(item);
			}
		}
		conexpr void insert_item_split(size_t pos, arr_block<T>& this_block, T&& item) {
			size_t block_size = this_block._size;
			size_t first_size = pos + 1;
			arr_block<T>& first_block = *new arr_block<T>(nullptr, first_size + 1, nullptr);
			arr_block<T>& second_block = *new arr_block<T>(&first_block, block_size - first_size, nullptr);

			for (size_t i = 0; i < pos; i++)
				first_block.arr_contain[i] = this_block.arr_contain[i];
			first_block[first_size] = std::move(item);
			for (size_t i = first_size; i < block_size; i++)
				second_block.arr_contain[i - first_size] = this_block.arr_contain[i];
			swap_block_with_blocks(this_block, first_block, second_block);
		}

		conexpr void insert_block_split(size_t pos, arr_block<T>& this_block, const T* item, size_t item_size) {
			size_t block_size = this_block._size;
			arr_block<T>& first_block = *new arr_block<T>(nullptr, pos, nullptr);
			arr_block<T>& second_block = *new arr_block<T>(nullptr, block_size - pos, nullptr);

			for (size_t i = 0; i < pos; i++)
				first_block.arr_contain[i] = this_block.arr_contain[i];

			for (size_t i = pos; i < block_size; i++)
				second_block.arr_contain[i - pos] = this_block.arr_contain[i];

			arr_block<T>& new_block_block = *new arr_block<T>(&first_block, item_size, &second_block);
			for (size_t i = 0; i < item_size; i++)
				new_block_block.arr_contain[i] = item[i];
			swap_block_with_blocks(this_block, first_block, second_block);
			_size += item_size;
		}


		conexpr size_t _remove_items(arr_block<T>* block, size_t start, size_t end) {
			size_t size = block->_size;
			size_t new_size = block->_size - (end - start);
			if (new_size == 0) {
				if (arr == block)
					arr = block->next_;
				if (arr_end == block)
					arr_end = block->_prev;
				block->good_bye_world();
			}
			else {
				T* new_arr = new T[new_size];
				T* arr_inter = block->arr_contain;
				size_t j = 0;
				for (size_t i = 0; j < new_size && i < size; i++) {
					if (i == start)
						i = end;
					new_arr[j++] = arr_inter[i];
				}
				delete[] arr_inter;
				block->arr_contain = new_arr;
				block->_size = new_size;
			}
			return size - new_size;
		}
		template<class _Fn>
		conexpr size_t _remove_if(arr_block<T>* block, _Fn func, size_t start, size_t end) {
			// value >> 3  ==  value / 8
			size_t size = block->_size;
			size_t rem_filt_siz =
				size > end ?
				(size >> 3) + (size & 7 ? 1 : 0) :
				(end >> 3) + (end & 7 ? 1 : 0);
			uint8_t* remove_filter = new uint8_t[rem_filt_siz]{ 0 };
			T* arr_inter = block->arr_contain;
			size_t new_size = size;

			for (size_t i = start; i < end; i++) {
				if (func(arr_inter[i])) {
					remove_filter[i >> 3] |= 1 << (i & 7);
					new_size--;
				}
			}
			if (size != new_size) {
				if (new_size == 0 && size == end) {
					if (arr == block)
						arr = block->next_;
					if (arr_end == block)
						arr_end = block->_prev;
					block->good_bye_world();
				}
				else {
					T* new_arr = new T[new_size];
					size_t j = 0;
					for (size_t i = 0; j < new_size && i < end; i++)
						if (!(remove_filter[i >> 3] & (1 << (i & 7))))
							new_arr[j++] = arr_inter[i];
					delete[] block->arr_contain;
					block->arr_contain = new_arr;
					block->_size = new_size;
				}
			}
			delete[] remove_filter;
			return size - new_size;
		}
	public:
		conexpr void clear() {
			arr_block<T>* blocks = arr;
			arr_block<T>* this_block;
			while (blocks != nullptr) {
				this_block = blocks;
				blocks = blocks->next_;
				this_block->_prev = nullptr;
				this_block->next_ = nullptr;
				delete this_block;
			}
			_size = 0;
			arr = arr_end = nullptr;
		}
		conexpr dynamic_arr() {}
		conexpr dynamic_arr(const dynamic_arr& copy) {
			operator=(copy);
		}
		conexpr dynamic_arr(dynamic_arr&& move) noexcept {
			operator=(std::move(move));
		}
		conexpr dynamic_arr& operator=(dynamic_arr&& move) noexcept {
			clear();
			arr = move.arr;
			arr_end = move.arr_end;
			_size = move._size;
			move.arr = move.arr_end = nullptr;
			return *this;
		}
		conexpr dynamic_arr& operator=(const dynamic_arr& copy) {
			T* tmp = (arr = arr_end = new arr_block<T>(nullptr, _size = copy._size, nullptr))->arr_contain;
			size_t i = 0;
			for (auto& it : copy)
				tmp[i++] = it;
			return *this;
		}
		conexpr ~dynamic_arr() {
			clear();
		}
		conexpr T& operator[](size_t pos) {
			return
				(pos < (_size >> 1)) ?
				arr->operator[](pos) :
				arr_end->index_back(_size - pos - 1)
				;
		}
		conexpr const T& operator[](size_t pos) const {
			return
				(pos < (_size >> 1)) ?
				arr->operator[](pos) :
				arr_end->index_back(_size - pos - 1)
				;
		}
		conexpr T& index_back(size_t pos) {
			return arr_end->index_back(_size - pos - 1);
		}
		conexpr const T& index_back(size_t pos) const {
			return arr_end->index_back(_size - pos - 1);
		}
		conexpr iterator<T> get_iterator(size_t pos) {
			if (pos < (_size >> 1))
				return arr->get_iterator(pos);
			else {
				if (_size - pos - 1 == size_t(-1))
					return iterator<T>(arr_end, arr_end ? arr_end->_size : 0);
				else
					return arr_end->get_iterator_back(_size - pos - 1);
			}
		}
		conexpr const_iterator<T> get_iterator(size_t pos) const {
			if (pos < (_size >> 1))
				return arr->get_iterator(pos);
			else {
				if (_size - pos - 1 == size_t(-1))
					return iterator<T>(arr_end, arr_end ? arr_end->_size : 0);
				else
					return arr_end->get_iterator_back(_size - pos - 1);
			}
		}
		conexpr auto begin() {
			return arr->begin();
		}
		conexpr auto end() {
			return arr_end->end();
		}
		conexpr auto begin() const {
			return arr->begin();
		}
		conexpr auto end() const {
			return arr_end->end();
		}
		conexpr size_t size() const {
			return _size;
		}
		conexpr void resize_begin(size_t new_size) {
			size_t tsize = _size;
			if (tsize >= new_size) {
				for (size_t resizer = tsize - new_size; resizer > 0;) {
					if (arr->_size > resizer) {
						_size = new_size;
						arr->resize_begin(arr->_size - resizer);
						return;
					}
					else {
						resizer -= arr->_size;
						if (auto tmp = arr->next_) {
							arr->next_->_prev = nullptr;
							arr->next_ = nullptr;
							delete arr;
							arr = tmp;
						}
						else {
							delete arr;
							arr_end = nullptr;
							arr = nullptr;
							_size = 0;
							return;
						}
					}
				}
			}
			else {
				if (arr)
					if (arr->_size + new_size <= _size >> 1) {
						arr->resize_begin(new_size - tsize + arr_end->_size);
						if (!arr_end) arr_end = arr;
						_size = new_size;
						return;
					}
				arr = new arr_block<T>(nullptr, new_size - tsize, arr);
				if (!arr_end) arr_end = arr;
			}
			_size = new_size;
		}
		conexpr void resize_front(size_t new_size) {
			size_t tsize = _size;
			if (tsize >= new_size) {
				for (size_t resizer = tsize - new_size; resizer > 0;) {
					if (arr_end->_size > resizer) {
						_size = new_size;
						arr_end->resize_front(arr_end->_size - resizer);
						return;
					}
					else {
						resizer -= arr_end->_size;
						if (auto tmp = arr_end->_prev) {
							arr_end->_prev->next_ = nullptr;
							arr_end->_prev = nullptr;
							delete arr_end;
							arr_end = tmp;
						}
						else {
							delete arr_end;
							arr_end = nullptr;
							arr = nullptr;
							_size = 0;
							return;
						}
					}
				}
			}
			else {
				if (arr_end)
					if (arr_end->_size + new_size <= _size >> 1) {
						arr_end->resize_front(new_size - tsize + arr_end->_size);
						if (!arr) arr = arr_end;
						_size = new_size;
						return;
					}
				arr_end = new arr_block<T>(arr_end, new_size - tsize, nullptr);
				if (!arr) arr = arr_end;
			}
			_size = new_size;
		}
		conexpr void insert_block(size_t pos, const T* item, size_t item_size) {
			if (pos == _size) {
				resize_front(_size + item_size);
				auto iterator = get_iterator(_size - item_size);
				for (size_t i = 0; i < item_size; i++) {
					*iterator = item[i];
					++iterator;
				}
			}
			else if (pos == 0) {
				resize_begin(_size + item_size);
				auto iterator = get_iterator(0);
				for (size_t i = 0; i < item_size; i++) {
					*iterator = item[i];
					++iterator;
				}
			}
			else
				insert_block_split(pos, *get_iterator(pos).block, item, item_size);
		}
		conexpr void insert_block(size_t pos, const arr_block<T>& item) {
			insert_block_split(pos, *get_iterator(pos).block, item.arr_contain, item._size);
		}
		conexpr void insert(size_t pos, const T& item) {
			if (!_size) {
				resize_front(1);
				operator[](0) = item;
				return;
			}
			iterator<T> inter = get_iterator(pos);
			arr_block<T>& this_block = *inter.block;
			if (inter.pos == 0) {
				this_block.resize_begin(this_block._size + 1);
				this_block[0] = item;
			}
			else if (inter.pos == this_block._size - 1) {
				this_block.resize_front(this_block._size + 1);
				this_block[this_block._size - 1] = item;
			}
			else if (this_block._size <= 50000)
				insert_item_slow(inter.pos, this_block, item);
			else
				insert_item_split(inter.pos, this_block, item);
			_size++;
		}
		conexpr void insert(size_t pos, T&& item) {
			if (!_size) {
				resize_front(1);
				operator[](0) = std::move(item);
				return;
			}
			iterator<T> inter = get_iterator(pos);
			arr_block<T>& this_block = *inter.block;
			if (inter.pos == 0) {
				this_block.resize_begin(this_block._size + 1);
				this_block[0] = std::move(item);
			}
			else if (inter.pos == this_block._size - 1) {
				this_block.resize_front(this_block._size + 1);
				this_block[this_block._size - 1] = std::move(item);
			}
			else if (this_block._size <= 50000)
				insert_item_slow(inter.pos, this_block, std::move(item));
			else
				insert_item_split(inter.pos, this_block, std::move(item));
			_size++;
		}
		conexpr void remove_item(size_t pos) {
			if (!_size)
				return;
			iterator<T> inter = get_iterator(pos);
			arr_block<T>& this_block = *inter.block;
			if (inter.pos == 0) {
				if (this_block._size == 1) {
					if (arr == &this_block)
						arr = this_block.next_;
					if (arr_end == &this_block)
						arr_end = this_block._prev;
					this_block.good_bye_world();
				}
				else
					this_block.resize_begin(this_block._size - 1);
			}
			else if (inter.pos == this_block._size - 1)
				this_block.resize_front(this_block._size - 1);
			else if (this_block._size <= 50000)
				remove_item_slow(inter.pos, this_block);
			else
				remove_item_split(inter.pos, this_block);
			_size--;
		}
		conexpr size_t remove_items(size_t start_pos, size_t end_pos) {
			iterator<T> interate = get_iterator(start_pos);
			iterator<T> _end = get_iterator(end_pos);
			size_t removed = 0;
			if (interate.block == _end.block) {
				removed = _remove_items(interate.block, interate.pos, _end.pos);
				_size -= removed;
				return removed;
			}

			removed += _remove_items(interate.block, interate.pos, interate.block->_size);
			for (;;) {
				if (!interate._nextBlock())
					break;
				removed += _remove_items(interate.block, 0, interate.block->_size);
			}

			if (_end.block)
				removed += _remove_items(_end.block, 0, _end.pos);
			else if (interate.block)
				removed += _remove_items(interate.block, 0, interate.block->_size);

			_size -= removed;
			return removed;
		}
		template<class _Fn>
		conexpr size_t remove_if(_Fn func, size_t start, size_t end) {
			if (!_size)
				return 0;
			iterator<T> interate = get_iterator(start);
			iterator<T> _end = get_iterator(end);
			size_t removed = 0;
			if (interate.block == _end.block) {
				removed = _remove_if(interate.block, func, interate.pos, _end.pos);
				_size -= removed;
				return removed;
			}

			removed += _remove_if(interate.block, func, interate.pos, interate.block->_size);
			for (;;) {
				if (!interate._nextBlock())
					break;
				removed += _remove_if(interate.block, func, 0, interate.block->_size);
			}

			if (_end.block)
				removed += _remove_if(_end.block, func, 0, _end.pos);
			else if (interate.block)
				removed += _remove_if(interate.block, func, 0, interate.block->_size);

			_size -= removed;
			return removed;
		}
		conexpr void swap(dynamic_arr<T>& to_swap) noexcept {
			arr_block<T>* old_arr = arr;
			arr_block<T>* old_arr_end = arr_end;
			size_t old__size = _size;
			arr = to_swap.arr;
			arr_end = to_swap.arr_end;
			_size = to_swap._size;
			to_swap.arr = old_arr;
			to_swap.arr_end = old_arr_end;
			to_swap._size = old__size;
		}
	};
	dynamic_arr<T> arr;
	size_t reserved_begin = 0;
	size_t _size = 0;
	size_t reserved_end = 0;
public:
	conexpr list_array() {}
	conexpr list_array(const std::initializer_list<T>& vals) {
		resize(vals.size());
		auto iter = begin();
		for (const T& it : vals) {
			*iter = it;
			++iter;
		}
	}
	conexpr list_array(const std::initializer_list<list_array<T>>& vals) {
		for (const list_array<T>& it : vals)
			push_back(it);
	}
	template<typename Iterable>
	conexpr list_array(Iterable begin, Iterable end, size_t reserve_len = 0) {
		if (reserve_len)
			reserve_push_back(reserve_len);
		while (begin != end)
			push_back(*begin++);
	}
	conexpr list_array(size_t size) {
		resize(size);
	}
	conexpr list_array(size_t size, const T& default_init) {
		resize(size, default_init);
	}
	conexpr list_array(list_array&& move) noexcept {
		operator=(std::move(move));
	}
	conexpr list_array(const list_array& copy) {
		operator=(copy);
	}
	conexpr list_array(const list_array& copy, size_t start, size_t end) {
		operator=(copy.copy(start, end));
	}
	conexpr list_array& operator=(list_array&& move) noexcept {
		arr = std::move(move.arr);
		reserved_begin = move.reserved_begin;
		_size = move._size;
		reserved_end = move.reserved_end;
		return *this;
	}
	conexpr list_array& operator=(const list_array& copy) {
		arr = copy.arr;
		reserved_begin = copy.reserved_begin;
		_size = copy._size;
		reserved_end = copy.reserved_end;
		return *this;
	}
	conexpr size_t alocated() const {
		return arr._size * sizeof(T);
	}
	conexpr size_t size() const {
		return _size;
	}
	template<bool do_shrink = false>
	conexpr void resize(size_t new_size) {
		static_assert(std::is_default_constructible<T>::value, "This type not default constructable");
		resize<do_shrink>(new_size, T());
	}
	template<bool do_shrink = false>
	conexpr void resize(size_t new_size, const T& auto_init) {
		if (new_size == 0) clear();
		else {
			if (reserved_end || !reserved_begin) arr.resize_front(reserved_begin + new_size);
			reserved_end = 0;
			if constexpr (do_shrink) {
				if (reserved_begin) arr.resize_begin(new_size);
				reserved_begin = 0;
			}
			for (auto& it : range(_size, new_size))
				it = auto_init;
		}
		_size = new_size;
	}

	conexpr void remove(size_t pos) {
		if (pos >= _size)
			throw std::out_of_range("pos value out of size limit");
		arr.remove_item(reserved_begin + pos);
		_size--;
		if (_size == 0) return clear();
	}
	conexpr void remove(size_t start_pos, size_t end_pos) {
		if (start_pos > end_pos)
			std::swap(start_pos, end_pos);
		if (end_pos > _size)
			throw std::out_of_range("end_pos value out of size limit");
		_size -= arr.remove_items(reserved_begin + start_pos, reserved_begin + end_pos);
	}

	conexpr void reserve_push_front(size_t reserve_size) {
		reserved_begin += reserve_size;
		arr.resize_begin(reserved_begin + _size + reserved_end);
	}
	conexpr void push_front(const T& copyer) {
		if (reserved_begin) {
			arr[--reserved_begin] = copyer;
			_size++;
		}
		else {
			reserve_push_front(_size + 1);
			push_front(copyer);
		}
	}
	conexpr void push_front(T&& copyer) {
		if (reserved_begin) {
			arr[--reserved_begin] = copyer;
			_size++;
		}
		else {
			reserve_push_front(_size + 1);
			push_front(std::move(copyer));
		}
	}

	conexpr void reserve_push_back(size_t reserve_size) {
		reserved_end += reserve_size;
		arr.resize_front(reserved_begin + _size + reserved_end);
	}
	conexpr void push_back(const T& copyer) {
		if (reserved_end) {
			arr[reserved_begin + _size++] = copyer;
			--reserved_end;
		}
		else {
			reserve_push_back(_size + 1);
			push_back(copyer);
		}
	}
	conexpr void push_back(T&& copyer) {
		if (reserved_end) {
			arr[reserved_begin + _size++] = std::move(copyer);
			--reserved_end;
		}
		else {
			reserve_push_back(_size + 1);
			push_back(std::move(copyer));
		}
	}

	conexpr void pop_back() {
		remove(_size - 1);
	}
	conexpr void pop_front() {
		remove(0);
	}

	conexpr T& back() {
		return operator[](_size - 1);
	}
	conexpr T& front() {
		return operator[](0);
	}
	conexpr const T& back() const {
		return operator[](_size - 1);
	}
	conexpr const T& front() const {
		return operator[](0);
	}


	conexpr void insert(size_t pos, const T* item, size_t arr_size) {
		if (!arr_size)
			return;
		arr.insert_block(reserved_begin + pos, item, arr_size);
		_size += arr_size;
	}
	conexpr void insert(size_t pos, const list_array<T>& item) {
		if (!item._size)
			return;
		T* as_array = item.to_array();
		arr.insert_block(reserved_begin + pos, as_array, item._size);
		_size += item._size;
		delete[] as_array;
	}
	conexpr void insert(size_t pos, const list_array<T>& item, size_t start, size_t end) {
		auto item_range = item.range(start, end);
		insert(pos, list_array<T>(item_range.begin(), item_range.end()));
	}
	conexpr void insert(size_t pos, const T& item) {
		if (pos == _size)
			return push_back(item);
		arr.insert(reserved_begin + pos, item);
		_size++;
	}
	conexpr void insert(size_t pos, T&& item) {
		if (pos == _size)
			return push_back(std::move(item));
		arr.insert(reserved_begin + pos, std::move(item));
		_size++;
	}

	conexpr void push_front(T* array, size_t arr_size) {
		insert(0, array, arr_size);
	}
	conexpr void push_back(T* array, size_t arr_size) {
		insert(_size, array, arr_size);
	}
	conexpr void push_front(const list_array<T>& to_push) {
		insert(0, to_push);
	}
	conexpr void push_back(const list_array<T>& to_push) {
		insert(_size, to_push);
	}

	//index optimization
	conexpr void commit() {
		T* tmp = new T[_size];
		begin()._fast_load(tmp, _size);
		arr.clear();
		arr.arr = arr.arr_end = new arr_block<T>();
		arr.arr->arr_contain = tmp;
		arr.arr->_size = _size;
		arr._size = _size;
	}
	//insert and remove optimization
	conexpr void decommit(size_t total_blocks) {
		if (total_blocks > _size)
			throw std::out_of_range("blocks count more than elements count");
		if (total_blocks == 0)
			throw std::out_of_range("blocks count cannont be 0");
		if (total_blocks == 1)
			return commit();
		list_array<T> tmp;
		size_t avg_block_len = _size / total_blocks;
		size_t last_block_add_len = _size % total_blocks;
		tmp.arr.arr = tmp.arr.arr_end = new arr_block<T>(nullptr, avg_block_len, nullptr);
		size_t block_iterator = 0;
		size_t new_total_blocks = 1;
		auto cur_iterator = begin();
		for (size_t i = 0; i < _size; i++) {
			if (block_iterator >= avg_block_len) {
				if (new_total_blocks >= total_blocks) {
					tmp.arr.arr_end->resize_front(tmp.arr.arr_end->_size + last_block_add_len);
					for (size_t j = 0; j < last_block_add_len; j++)
						tmp.arr.arr_end->arr_contain[avg_block_len + j] = operator[](i++);
					break;
				}
				else {
					block_iterator = 0;
					new_total_blocks++;
					tmp.arr.arr_end = new arr_block<T>(tmp.arr.arr_end, avg_block_len, nullptr);
				}
			}
			tmp.arr.arr_end->arr_contain[block_iterator++] = (*cur_iterator);
			++cur_iterator;
		}
		arr.clear();
		arr.arr = tmp.arr.arr;
		arr.arr_end = tmp.arr.arr_end;
		tmp.arr.arr = tmp.arr.arr_end = nullptr;
		arr._size = _size;
		reserved_begin = reserved_end = 0;
	}
	conexpr bool need_commit() const {
		return arr.arr != arr.arr_end && arr.arr->next_ != arr.arr_end;
	}
	conexpr bool blocks_more(size_t blocks_count) const {
		const arr_block<T>* block = arr.arr;
		size_t res = 0;
		while (block) {
			if (++res > blocks_count)
				return true;
			block = block->next_;
		}
		return false;
	}
	size_t blocks_count() const {
		const arr_block<T>* block = arr.arr;
		size_t res = 0;
		while (block) {
			++res;
			block = block->next_;
		}
		return res;
	}
	//remove reserved memory
	conexpr void shrink_to_fit() {
		resize<true>(_size);
	}


	conexpr bool contains(const T& value) const req(std::equality_comparable<T>) {
		return contains(value, 0, _size);
	}
	conexpr bool contains(const T& value, size_t start, size_t end) const req(std::equality_comparable<T>) {
		for (const T& it : range(start, end))
			if (it == value)
				return true;
		return false;
	}

	conexpr bool contains(const list_array<T>& value) const req(std::equality_comparable<T>) {
		return contains(value, 0, value._size, 0, _size);
	}
	conexpr bool contains(const list_array<T>& value, size_t start, size_t end) const req(std::equality_comparable<T>) {
		return contains(value, 0, value._size, start, end);
	}
	conexpr bool contains(const list_array<T>& value, size_t value_start, size_t value_end, size_t start, size_t end) const req(std::equality_comparable<T>) {
		if (value_end > value._size)
			throw std::out_of_range("value_end is out of value size limit");
		auto vbeg = value.get_iterator(value_start);
		auto ibeg = vbeg;
		size_t i = value_start;
		for (const T& it : range(start, end)) {
			if (it == *ibeg) {
				++i;
				if (i == value_end)
					return true;
				++ibeg;
			}
			else {
				i = value_start;
				ibeg = vbeg;
			}
		}
		return false;
	}

	template<class _Fn>
	conexpr bool contains_one(_Fn check_functon) const {
		return contains_one(check_functon, 0, _size);
	}
	template<class _Fn>
	conexpr size_t contains_multiply(_Fn check_functon) const {
		return contains_multiply(check_functon, 0, _size);
	}
	template<class _Fn>
	conexpr bool contains_one(_Fn check_functon, size_t start, size_t end) const {
		for (const T& it : range(start, end))
			if (check_functon(it))
				return true;
		return false;
	}
	template<class _Fn>
	conexpr size_t contains_multiply(_Fn check_functon, size_t start, size_t end) const {
		size_t i = 0;
		for (const T& it : range(start, end))
			if (check_functon(it))
				++i;
		return i;
	}

	template<class _Fn>
	conexpr size_t remove_if(_Fn check_function) {
		size_t res = arr.remove_if(check_function, reserved_begin, reserved_begin + _size);
		_size -= res;
		return res;
	}
	template<class _Fn>
	conexpr size_t remove_if(_Fn check_function, size_t start, size_t end) {
		if (start > end)
			std::swap(start, end);
		if (end > _size)
			throw std::out_of_range("end value out of size limit");
		if (start > _size)
			throw std::out_of_range("start value out of size limit");
		size_t res = arr.remove_if(check_function, reserved_begin + start, reserved_begin + end);
		_size -= res;
		return res;
	}

	template<class _Fn>
	conexpr size_t remove_same(T val, size_t start, size_t end, _Fn comparer = [](T f, T s) { return f == s; }) {
		if (start > end)
			std::swap(start, end);
		if (end > _size)
			throw std::out_of_range("end value out of size limit");
		if (start > _size)
			throw std::out_of_range("start value out of size limit");
		size_t res = arr.remove_if([](T cval) { return comparer(val, cval); }, reserved_begin + start, reserved_begin + end);
		_size -= res;
		return res;
	}
	template<class _Fn>
	conexpr size_t remove_same(T val, _Fn comparer) {
		size_t res = arr.remove_if([](T cval) { return comparer(val, cval); }, reserved_begin, reserved_begin + _size);
		_size -= res;
		return res;
	}

	conexpr list_array<T> flip_copy() const {
		list_array<T> larr(_size);
		size_t i = 0;
		for (auto item : reverse())
			larr[i++] = item;
		return larr;
	}
	conexpr list_array<T>& flip() {
		operator=(flip_copy());
		return *this;
	}

	conexpr reverse_provider reverse() {
		return *this;
	}
	conexpr const_reverse_provider reverse() const {
		return *this;
	}

	conexpr range_provider range(size_t start, size_t end) {
		return range_provider(*this, start, end);
	}
	conexpr reverse_provider reverse_range(size_t start, size_t end) {
		return range_provider(*this, start, end);
	}
	conexpr const_range_provider range(size_t start, size_t end) const {
		return const_range_provider(*this, start, end);
	}
	conexpr const_reverse_provider reverse_range(size_t start, size_t end) const {
		return const_range_provider(*this, start, end);
	}

	conexpr size_t find(const T& value) const req(std::equality_comparable<T>) {
		return find(value, 0);
	}
	conexpr size_t find(const T& value, size_t continue_from) const req(std::equality_comparable<T>) {
		size_t i = 0;
		for (auto& it : range(continue_from, _size)) {
			if (it == value)
				return i;
			++i;
		}
		return npos;
	}
	conexpr size_t find(const T* vbeg, const T* vend) const req(std::equality_comparable<T>) {
		return find(vbeg, vend, 0);
	}
	conexpr size_t find(const T* vbeg, const T* vend, size_t continue_from) const req(std::equality_comparable<T>) {
		size_t i = 0;
		size_t eq = 0;
		size_t dif = vend - vbeg;

		for (auto& it : range(continue_from, _size)) {
			if (vbeg[eq] == it)
				++eq;
			else
				eq = 0;
			if (eq == dif)
				return i;
			++i;
		}
		return npos;
	}
	conexpr size_t find(const list_array<T>& value) const req(std::equality_comparable<T>) {
		return find(value, 0, value._size, 0, _size);
	}
	conexpr size_t find(const list_array<T>& value, size_t continue_from) const req(std::equality_comparable<T>) {
		return find(value, 0, value._size, continue_from, _size);
	}
	conexpr size_t find(const list_array<T>& value, size_t continue_from, size_t end_pos) const req(std::equality_comparable<T>) {
		return find(value, 0, value._size, continue_from, end_pos);
	}
	conexpr size_t find(const list_array<T>& value, size_t value_start, size_t value_end, size_t continue_from, size_t end) const req(std::equality_comparable<T>) {
		if (!value._size)
			throw std::out_of_range("this value too small for searching value");
		if (value_start > value_end) {
			std::swap(value_end, value_start);
			if (value_end > value._size)
				throw std::out_of_range("value_end is out of value size limit");
			auto vbeg = const_reverse_iterator<T>(value.get_iterator(value_end));
			auto ibeg = vbeg;
			size_t i = value_start;
			size_t res = continue_from;
			for (const T& it : range(continue_from, end)) {
				if (it == *ibeg) {
					++i;
					if (i == value_end)
						return res;
					++ibeg;
				}
				else {
					i = value_start;
					ibeg = vbeg;
				}
				++res;
			}
		}
		else {
			if (value_end > value._size)
				throw std::out_of_range("value_end is out of value size limit");
			auto vbeg = value.get_iterator(value_start);
			auto ibeg = vbeg;
			size_t i = value_start;
			size_t res = continue_from;
			for (const T& it : range(continue_from, end)) {
				if (it == *ibeg) {
					++i;
					if (i == value_end)
						return res;
					++ibeg;
				}
				else {
					i = value_start;
					ibeg = vbeg;
				}
				++res;
			}
		}
		return npos;
	}

	conexpr size_t findr(const T& value) const req(std::equality_comparable<T>) {
		return findr(value, _size);
	}
	conexpr size_t findr(const T& value, size_t continue_from) const req(std::equality_comparable<T>) {
		size_t i = 0;
		for (auto& it : reverse_range(0, continue_from)) {
			if (it == value)
				return i;
			++i;
		}
		return npos;
	}
	conexpr size_t findr(const T* vbeg, const T* vend) const req(std::equality_comparable<T>) {
		return findr(vbeg, vend, _size);
	}
	conexpr size_t findr(const T* vbeg, const T* vend, size_t continue_from) const req(std::equality_comparable<T>) {
		size_t i = 0;
		size_t eq = 0;
		size_t dif = vend - vbeg;

		for (auto& it : reverse_range(0, continue_from)) {
			if (vbeg[eq] == it)
				++eq;
			else
				eq = 0;
			if (eq == dif)
				return i;
			++i;
		}
		return npos;
	}
	conexpr size_t findr(const list_array<T>& value) const req(std::equality_comparable<T>) {
		return findr(value, 0, value._size, 0, _size);
	}
	conexpr size_t findr(const list_array<T>& value, size_t continue_from) const req(std::equality_comparable<T>) {
		return findr(value, 0, value._size, continue_from, _size);
	}
	conexpr size_t findr(const list_array<T>& value, size_t continue_from, size_t end_pos) const req(std::equality_comparable<T>) {
		return findr(value, 0, value._size, continue_from, end_pos);
	}
	conexpr size_t findr(const list_array<T>& value, size_t value_start, size_t value_end, size_t continue_from, size_t end) const req(std::equality_comparable<T>) {
		if (!value._size)
			throw std::out_of_range("this value too small for searching value");
		if (value_start > value_end) {
			if (value_end > value._size)
				throw std::out_of_range("value_end is out of value size limit");
			auto vbeg = const_reverse_iterator<T>(value.get_iterator(value_end));
			auto ibeg = vbeg;
			size_t i = value_start;
			size_t res = continue_from;
			for (const T& it : reverse_range(end, continue_from)) {
				if (it == *ibeg) {
					++i;
					if (i == value_end)
						return res;
					++ibeg;
				}
				else {
					i = value_start;
					ibeg = vbeg;
				}
				++res;
			}
		}
		else {
			if (value_end > value._size)
				throw std::out_of_range("value_end is out of value size limit");
			auto vbeg = value.get_iterator(value_start);
			auto ibeg = vbeg;
			size_t i = value_start;
			size_t res = continue_from;
			for (const T& it : reverse_range(end, continue_from)) {
				if (it == *ibeg) {
					++i;
					if (i == value_end)
						return res;
					++ibeg;
				}
				else {
					i = value_start;
					ibeg = vbeg;
				}
				++res;
			}
		}
		return npos;
	}

	template<class _Fn>
	conexpr size_t find_it(_Fn find_func) const {
		return find_it(find_func, 0);
	}
	template<class _Fn>
	conexpr size_t find_it(_Fn find_func, size_t continue_from) const {
		size_t i = 0;
		for (auto& it : range(continue_from, _size)) {
			if (find_func(it))
				return i;
			++i;
		}
		return npos;
	}

	template<class _Fn>
	conexpr size_t findr_it(_Fn find_func) const {
		return findr_it(find_func, _size);
	}
	template<class _Fn>
	conexpr size_t findr_it(_Fn find_func, size_t continue_from) const {
		size_t i = 0;
		for (T& it : reverse_range(0, continue_from)) {
			if (find_func(it))
				return i;
			++i;
		}
		return npos;
	}

	conexpr void clear() {
		arr.clear();
		_size = reserved_end = reserved_begin = 0;
	}

	conexpr list_array<T> sort_copy() const {
		return list_array<T>(*this).sort();
	}
	conexpr list_array<T>& sort() {
		if constexpr (std::is_unsigned<T>::value) {
			const T& mival = mmin();
			size_t dif = mmax() - mival + 1;
			list_array<size_t> count_arr(dif);
			list_array<T> result(_size);
			{
				size_t i = 0;
				for (const T& it : *this)
					count_arr[it - mival] += 1;
			}
			for (size_t i = 1; i < dif; i++)
				count_arr[i] += count_arr[i - 1];
			{
				for (const T& it : reverse()) {
					result[count_arr[it - mival] - 1] = it;
					count_arr[it - mival] -= 1;
				}
			}
			swap(result);
		}
		else if constexpr (std::is_signed<T>::value && !sizeof(T) < sizeof(size_t)) {
			auto normalize = [](const T& to) {
				constexpr size_t to_shift = sizeof(8) * 4;
				return size_t((SIZE_MAX >> to_shift) + to);
			};
			size_t mival = normalize(mmin());
			size_t dif = normalize(mmax()) - mival + 1;
			list_array<size_t> count_arr(dif, 0);
			list_array<T> result(_size);
			{
				size_t i = 0;
				for (const T& it : *this)
					count_arr[normalize(it) - mival] += 1;
			}
			for (size_t i = 1; i < dif; i++)
				count_arr[i] += count_arr[i - 1];
			{
				for (const T& it : reverse()) {
					result[count_arr[normalize(it) - mival] - 1] = it;
					count_arr[normalize(it) - mival] -= 1;
				}
			}
			swap(result);
		}
		else {
			size_t curr_L_size = _size / 2 + 1;
			size_t curr_M_size = _size / 2 + 1;
			T* L = new T[_size / 2 + 1];
			T* M = new T[_size / 2 + 1];
			auto fix_size = [&L, &M, &curr_L_size, &curr_M_size](size_t start, size_t mindle, size_t end) {
				size_t l = mindle - start + 1;
				size_t m = end - mindle + 1;
				if (curr_L_size < l) {
					delete[] L;
					L = new T[l];
					curr_L_size = l;
				}
				if (curr_M_size < m) {
					delete[] M;
					M = new T[m];
					curr_M_size = m;
				}
			};
			auto merge = [&](size_t start, size_t mindle, size_t end) {
				size_t n1 = mindle - start;
				size_t n2 = end - mindle;
				if (curr_L_size < n1 || curr_M_size < n2)
					fix_size(start, mindle, end);
				get_iterator(start)._fast_load(L, n1);
				get_iterator(mindle)._fast_load(M, n2);
				size_t i = 0, j = 0, k = start;
				for (T& it : range(start, end)) {
					if (i < n1 && j < n2)
						it = L[i] <= M[j] ? L[i++] : M[j++];
					else if (i < n1)
						it = L[i++];
					else if (j < n2)
						it = M[j++];
				}
			};
			for (size_t b = 2; b < _size; b <<= 1) {
				for (size_t i = 0; i < _size; i += b) {
					if (i + b > _size)
						merge(i, i + ((_size - i) >> 1), _size);
					else
						merge(i, i + (b >> 1), i + b);
				}
				if (b << 1 > _size)
					merge(0, b, _size);
			}
			auto non_sorted_finder = [&](size_t continue_search) {
				const auto* check = &operator[](0);
				size_t res = 0;
				for (const T& it : *this) {
					if (*check > it)
						return res;
					check = &it;
					++res;
				}
				return size_t(0);
			};
			size_t err = 0;
			while (err = non_sorted_finder(err))
				merge(0, err, _size);
			delete[] L;
			delete[] M;
		}
		return *this;
	}

	conexpr const T& mmax() const {
		if (!_size)
			throw std::length_error("This list_array size is zero");
		const T* max = &operator[](0);
		for (const T& it : *this)
			if (*max < it)
				max = &it;
		return *max;
	}
	conexpr const T& mmin() const {
		if (!_size)
			throw std::length_error("This list_array size is zero");
		const T* min = &operator[](0);
		for (const T& it : *this)
			if (*min > it)
				min = &it;
		return *min;
	}
	conexpr T max_default() const req(std::copy_constructible<T>) {
		if (!_size)
			return T();
		const T* max = &operator[](0);
		for (const T& it : *this)
			if (*max < it)
				max = &it;
		return *max;
	}
	conexpr T min_default() const req(std::copy_constructible<T>) {
		if (!_size)
			return T();
		const T* min = &operator[](0);
		for (const T& it : *this)
			if (*min > it)
				min = &it;
		return *min;
	}

	conexpr void swap(list_array<T>& to_swap) noexcept {
		if (arr.arr != to_swap.arr.arr) {
			arr.swap(to_swap.arr);
			size_t rb = reserved_begin;
			size_t re = reserved_end;
			size_t s = _size;
			reserved_begin = to_swap.reserved_begin;
			_size = to_swap._size;
			reserved_end = to_swap.reserved_end;
			to_swap.reserved_begin = rb;
			to_swap.reserved_end = re;
			to_swap._size = s;
		}
	}
	conexpr bool operator==(const list_array<T>& to_cmp) const {
		if (arr.arr != to_cmp.arr.arr) {
			if (_size != to_cmp._size)
				return false;
			auto iter = to_cmp.begin();
			for (const T& it : *this) {
				if (*iter != it)
					return false;
				++iter;
			}
		}
		return true;
	}
	conexpr bool operator!=(const list_array<T>& to_cmp) const {
		return !operator==(to_cmp);
	}

	conexpr list_array<T> split(size_t split_pos) {
		if (_size <= split_pos)
			throw std::out_of_range("Fail split due small array or split_pos is equal with array size");
		list_array<T> res(_size - split_pos);
		size_t i = 0;
		for (auto& it : range(split_pos, _size))
			res[i++] = std::move(it);
		remove(split_pos, _size);
		return res;
	}
	conexpr T take(size_t take_pos) {
		if (_size <= take_pos)
			throw std::out_of_range("Fail take item due small array");
		T res(std::move(operator[](take_pos)));
		remove(take_pos);
		return res;
	}
	conexpr list_array<T> take(size_t start_pos, size_t end_pos) {
		if (start_pos > end_pos) {
			std::swap(start_pos, end_pos);
			if (_size < end_pos)
				throw std::out_of_range("Fail take items due small array");
			list_array<T> res(end_pos - start_pos);
			size_t i = 0;
			for (auto& it : reverse_range(start_pos, end_pos))
				res[i++] = std::move(it);
			remove(start_pos, end_pos);
			return res;
		}
		else {
			if (_size < end_pos)
				throw std::out_of_range("Fail take items due small array");
			list_array<T> res(end_pos - start_pos);
			size_t i = 0;
			for (auto& it : range(start_pos, end_pos))
				res[i++] = std::move(it);
			remove(start_pos, end_pos);
			return res;
		}
	}
	template<class _Fn>
	conexpr list_array<T> take(_Fn select_fn) {
		return take(select_fn, 0, _size);
	}
	template<class _Fn>
	conexpr list_array<T> take(_Fn select_fn, size_t start_pos, size_t end_pos) {
		size_t i = 0;
		size_t taken_items = 0;
		list_array<uint8_t> selector((end_pos - start_pos) >> 3 + 1, 0);
		if (start_pos > end_pos) {
			std::swap(start_pos, end_pos);
			if (_size < end_pos)
				throw std::out_of_range("Fail take items due small array");
			for (auto& it : range(start_pos, end_pos)) {
				if (select_fn(it)) {
					++taken_items;
					selector[i >> 3] |= 1 << (i & 7);
				}
				i++;
			}
			if (taken_items == 0)
				return {};
			list_array<T> res;
			res.reserve_push_back(taken_items);
			for (auto& it : reverse_range(start_pos, end_pos)) {
				--i;
				if (selector[i >> 3] & (1 << (i & 7)))
					res.push_back(it);
			}
			remove_if(
				[selector, &i]() {
					bool res = selector[i >> 3] & (1 << (i & 7));
					i++;
					return res;
				},
				start_pos,
					end_pos
					);
			return res;
		}
		else {
			if (_size < end_pos)
				throw std::out_of_range("Fail take items due small array");
			for (auto& it : range(start_pos, end_pos)) {
				if (select_fn(it)) {
					++taken_items;
					selector[i >> 3] |= 1 << (i & 7);
				}
				i++;
			}
			if (taken_items == 0)
				return {};
			list_array<T> res;
			res.reserve_push_back(taken_items);
			i = 0;
			for (auto& it : range(start_pos, end_pos)) {
				if (selector[i >> 3] & (1 << (i & 7)))
					res.push_back(it);
			}
			i = 0;
			remove_if(
				[selector, &i]() {
					bool res = selector[i >> 3] & (1 << (i & 7));
					i++;
					return res;
				},
				start_pos,
					end_pos
					);
			return res;
		}
	}

	conexpr list_array<T> copy(size_t start_pos, size_t end_pos) const {
		if (start_pos > end_pos) {
			std::swap(start_pos, end_pos);
			if (_size < end_pos)
				throw std::out_of_range("Fail take items due small array");
			list_array<T> res(end_pos - start_pos);
			size_t i = 0;
			for (auto& it : reverse_range(start_pos, end_pos))
				res[i++] = it;
			return res;
		}
		else {
			if (_size < end_pos)
				throw std::out_of_range("Fail take items due small array");
			list_array<T> res(end_pos - start_pos);
			size_t i = 0;
			for (auto& it : range(start_pos, end_pos))
				res[i++] = it;
			return res;
		}
	}
	conexpr list_array<T> copy() const {
		return *this;
	}

	conexpr size_t unique() {
		return unique(0, _size);
	}
	conexpr size_t unique(size_t start_pos, size_t end_pos) {
		if (start_pos > end_pos)
			std::swap(start_pos, end_pos);
		if (start_pos + 1 >= end_pos)
			return 0;
		if (end_pos > _size)
			throw std::out_of_range("end_pos out of size limit");
		T* it = &operator[](start_pos);
		size_t res = 0;
		remove_if(
			[&it, &res](T& check_it) {
				if (check_it == *it)
					return (bool)++res;
				it = &check_it;
				return false;
			},
			start_pos + 1,
				end_pos
				);
		return res;
	}

	//keep only unique item neighbors
	template<class _Fn>
	conexpr size_t unique(_Fn compare_func) {
		return unique(compare_func, 0, _size);
	}
	template<class _Fn>
	conexpr size_t unique(_Fn compare_func, size_t start_pos, size_t end_pos) {
		if (start_pos > end_pos)
			std::swap(start_pos, end_pos);
		if (start_pos + 1 >= end_pos)
			return 0;
		if (end_pos > _size)
			throw std::out_of_range("end_pos out of size limit");
		T* it = &operator[](start_pos);
		size_t res = 0;
		remove_if(
			[&it, &res, &compare_func](T& check_it) {
				if (compare_func(*it, check_it))
					return (bool)++res;
				it = &check_it;
				return false;
			},
			start_pos + 1,
				end_pos
				);
		return res;
	}

	//remove all copys
	conexpr size_t unify() {
		return unify(0, _size);
	}
	conexpr size_t unify(size_t start_pos, size_t end_pos) {
		list_array<T> tmp_arr;
		tmp_arr.reserve_push_back((_size >> 2) + 1);
		for (T& it : range(start_pos, end_pos))
			if (!tmp_arr.contains(it))
				tmp_arr.push_back(it);
		tmp_arr.shrink_to_fit();
		swap(tmp_arr);
		return tmp_arr._size - _size;
	}

	//keep only unique from all array
	conexpr size_t alone() {
		return alone(0, _size);
	}
	conexpr size_t alone(size_t start_pos, size_t end_pos) {
		if (start_pos > end_pos)
			std::swap(start_pos, end_pos);
		if (start_pos + 1 >= end_pos)
			return 0;
		if (end_pos > _size)
			throw std::out_of_range("end_pos out of size limit");
		uint8_t* selector = new uint8_t[((end_pos - start_pos) >> 3) + 1]{ 0 };
		size_t i = 0;
		for (T& it : range(start_pos, end_pos)) {
			if (selector[i >> 3] & (1 << (i & 7))) {
				i++;
				continue;
			}
			size_t j = 0;
			bool is_unique = true;
			for (T& cmp_it : range(start_pos, end_pos)) {
				if (i == j) {
					j++;
					continue;
				}
				if (it == cmp_it) {
					is_unique = false;
					selector[j >> 3] |= (1 << (j & 7));
				}
				j++;
			}
			if (!is_unique)
				selector[i >> 3] |= (1 << (i & 7));
			i++;
		}
		i = 0;
		size_t result =
			remove_if(
				[selector, &i](T& check_it) {
					bool res = selector[i >> 3] & (1 << (i & 7));
					i++;
					return res;
				},
				start_pos,
					end_pos
					);
		delete[] selector;
		return result;
	}

	conexpr void join(const T& insert_item) {
		operator=(join_copy(insert_item, 0, _size));
	}
	conexpr list_array<T> join_copy(const T& insert_item) const {
		return join_copy(insert_item, 0, _size);
	}
	conexpr list_array<T> join_copy(const T& insert_item, size_t start_pos, size_t end_pos) const {
		list_array<T> res;
		res.reserve_push_back(_size * 2);
		if (start_pos > end_pos) {
			std::swap(start_pos, end_pos);
			if (end_pos > _size)
				throw std::out_of_range("end_pos out of size limit");
			for (auto& i : reverse_range(start_pos, end_pos)) {
				res.push_back(i);
				res.push_back(insert_item);
			}
		}
		else {
			if (end_pos > _size)
				throw std::out_of_range("end_pos out of size limit");
			for (auto& i : range(start_pos, end_pos)) {
				res.push_back(i);
				res.push_back(insert_item);
			}
		}
		return res;
	}

	template<class _Fn>
	conexpr list_array<T> where(_Fn check_fn) const {
		return where_copy(check_fn, 0, _size);
	}
	template<class _Fn>
	conexpr list_array<T> where(_Fn check_fn, size_t start_pos, size_t end_pos) const {
		list_array<T> res;
		res.reserve_push_back(_size);
		if (start_pos > end_pos) {
			std::swap(start_pos, end_pos);
			if (end_pos > _size)
				throw std::out_of_range("end_pos out of size limit");
			for (auto& i : reverse_range(start_pos, end_pos))
				if (check_fn(i))
					res.push_back(i);
		}
		else {
			if (end_pos > _size)
				throw std::out_of_range("end_pos out of size limit");
			for (auto& i : range(start_pos, end_pos))
				if (check_fn(i))
					res.push_back(i);
		}
		res.shrink_to_fit();
		return res;
	}

	template<class ConvertTo, class _Fn>
	conexpr list_array<ConvertTo> convert(_Fn iterate_fn) {
		return convert<ConvertTo>(iterate_fn, 0, _size);
	}
	template<class ConvertTo, class _Fn>
	conexpr list_array<ConvertTo> convert(_Fn iterate_fn, size_t start_pos, size_t end_pos) {
		list_array<ConvertTo> res;
		res.reserve_push_back(_size);
		if (start_pos > end_pos) {
			std::swap(start_pos, end_pos);
			if (end_pos > _size)
				throw std::out_of_range("end_pos out of size limit");
			for (auto& i : reverse_range(start_pos, end_pos))
				res.push_back(iterate_fn(i));
		}
		else {
			if (end_pos > _size)
				throw std::out_of_range("end_pos out of size limit");
			for (auto& i : range(start_pos, end_pos))
				res.push_back(iterate_fn(i));
		}
		return res;
	}

	conexpr void erase(const T& val) {
		remove_if([&val](const T& cmp) { return cmp == val; });
	}
	conexpr void erase(const T& val, size_t start_pos, size_t end_pos) {
		remove_if([&val](const T& cmp) { return cmp == val; }, start_pos, end_pos);
	}
	conexpr void erase(const list_array<T>& range) {
		erase(range, 0, range._size, 0, _size);
	}
	conexpr void erase(const list_array<T>& range, size_t start_pos, size_t end_pos) {
		erase(range, 0, range._size, start_pos, end_pos);
	}
	conexpr void erase(const list_array<T>& range, size_t range_start, size_t range_end, size_t start_pos, size_t end_pos) {
		list_array<size_t> remove_pos;
		size_t range_size = range_start > range_end ? range_start - range_end : range_end - range_start;
		size_t find_item = start_pos;
		while (find_item = find(range, range_start, range_end, find_item, end_pos) != npos)
			remove_pos.push_back(find_item - range_size);
		for (size_t val : remove_pos) { remove(val, range_size); }
	}

	conexpr iterator<T> get_iterator(size_t pos) {
		return arr.get_iterator(reserved_begin + pos);
	}
	conexpr const_iterator<T> get_iterator(size_t pos) const {
		return arr.get_iterator(reserved_begin + pos);
	}
	conexpr iterator<T> begin() {
		return arr.get_iterator(reserved_begin);
	}
	conexpr iterator<T> end() {
		return arr.get_iterator(reserved_begin + _size);
	}
	conexpr const_iterator<T> begin() const {
		return arr.get_iterator(reserved_begin);
	}
	conexpr const_iterator<T> end() const {
		return arr.get_iterator(reserved_begin + _size);
	}

	conexpr reverse_iterator<T> rbegin() {
		return arr.get_iterator(reserved_begin + _size);
	}
	conexpr reverse_iterator<T> rend() {
		return arr.get_iterator(reserved_begin);
	}
	conexpr const_reverse_iterator<T> rbegin() const {
		return arr.get_iterator(reserved_begin + _size);
	}
	conexpr const_reverse_iterator<T> rend() const {
		return arr.get_iterator(reserved_begin);
	}

	conexpr const_iterator<T> cbegin() const {
		return arr.get_iterator(reserved_begin);
	}
	conexpr const_iterator<T> cend() const {
		return arr.get_iterator(reserved_begin + _size);
	}
	conexpr const_reverse_iterator<T> crbegin() const {
		return arr.get_iterator(reserved_begin + _size);
	}
	conexpr const_reverse_iterator<T> crend() const {
		return arr.get_iterator(reserved_begin);
	}

	conexpr inline T& operator[](size_t pos) {
		return arr[reserved_begin + pos];
	}
	conexpr inline const T& operator[](size_t pos) const {
		return arr[reserved_begin + pos];
	}
	conexpr T& at(size_t pos) {
		if (pos >= _size)
			throw std::out_of_range("pos out of size limit");
		return arr[reserved_begin + pos];
	}
	conexpr const T& at(size_t pos) const {
		if (pos >= _size)
			throw std::out_of_range("pos out of size limit");
		return arr[reserved_begin + pos];
	}
	conexpr T atDefault(size_t pos) const {
		if (pos >= _size)
			return T();
		return arr[reserved_begin + pos];
	}

	conexpr T* to_array() const {
		T* tmp = new T[_size];
		begin()._fast_load(tmp, _size);
		return tmp;
	}
};
#undef req
#undef conexpr
#endif