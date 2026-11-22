.PHONY: all

tests := print foobar overload-extern simple-template higher-order-template \
				 defered-higher-order-template define-with-template-on-rhs

test_targets := $(foreach t,$(tests),test_$(t) )
all: $(test_targets)

opium_flags := -L ../../pl/libs \
						   -L ../

test_%: %.osl
	@ echo -n "test $^ ... "
	@ opium $(opium_flags) -o out/$@.scm $^ && chicken-csi -s out/$@.scm >out/$@.out
	@ diff out/$@.out expect/$^.txt

