.PHONY: all

tests := print foobar overload-extern simple-template higher-order-template \
				 defered-higher-order-template define-with-template-on-rhs

test_targets := $(foreach t,$(tests),test_$(t) )
all: $(test_targets)

opium_flags := -L ../../pl/libs \
						   -L ../

K := \e[38;5;2;1mok\e[0m
F := \e[38;5;1;1mfailed\e[0m

test_%: %.osl
	@ echo -e "test \e[38;5;4;1m$^\e[0m"
	@ echo -n "  compile .. " \
	 	&& opium $(opium_flags) -o out/$@.scm $^ &>out/$@.log \
		&& chicken-csi -s out/$@.scm >out/$@.out \
		&& echo -e "$(K) [$$(paste -d'; ' out/$@.log)]" \
		|| echo -e "$(F)"
	@ test expect/$^.txt && echo -n "  output .. " && ( \
			diff out/$@.out expect/$^.txt &>/dev/null \
			&& echo -e "$(K)" \
			|| echo -e "$(F)" \
		) || :

