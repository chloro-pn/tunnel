#ifndef TUNNEL_UTIL_H
#define TUNNEL_UTIL_H

namespace tunnel {

class Endian {
 public:
  enum class Type { Big, Little };

  static Endian& Instance() {
    static Endian obj;
    return obj;
  }

  Type GetEndianType() const { return type_; }

 private:
  Endian() {
    int i = 1;
    type_ = (*(char*)&i == 1) ? Type::Little : Type::Big;
  }

  Type type_;
};

template <typename T>
inline T flipByByte(T t) {
  T ret{0};
  char* ptr1 = reinterpret_cast<char*>(&ret);
  char* ptr2 = reinterpret_cast<char*>(&t);
  for (int i = 0; i < sizeof(T); ++i) {
    int j = sizeof(T) - 1 - i;
    ptr1[j] = ptr2[i];
  }
  return ret;
}

}  // namespace tunnel

#endif
