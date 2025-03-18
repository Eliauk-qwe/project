all: mytbf

# 显式定义生成 mytbf.o 的规则（避免隐式规则失效）
mytbf: main.o mytbf.o
	gcc $^ -o $@

# 从 mytbf.c 生成 mytbf.o
mytbf.o: mytbf.c mytbf.h
	gcc -c $< -o $@

# 从 main.c 生成 main.o
main.o: main.c mytbf.h
	gcc -c $< -o $@

clean:
	rm -rf *.o mytbf