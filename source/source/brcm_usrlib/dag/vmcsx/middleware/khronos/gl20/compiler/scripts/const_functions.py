import re

class proto_group:
	pass

angle_and_trig = proto_group()
angle_and_trig.name = "Angle and Trigonometry Functions"
angle_and_trig.change_dims = (1,2,3,4)
angle_and_trig.changes = [
	("genType", {1 :"const_float", 2 :"const_vec2", 3 :"const_vec3", 4 :"const_vec4"}),
]
angle_and_trig.protos = [
	"genType radians(genType degrees)",
	"genType degrees(genType radians)",
	"genType sin(genType angle)",
	"genType cos(genType angle)",
	"genType tan(genType angle)",
	"genType asin(genType x)",
	"genType acos(genType x)",
	"genType atan(genType y, genType x)",
	"genType atan(genType y_over_x)",
]

exponential = proto_group()
exponential.name ="Exponential Functions"
exponential.change_dims = (1,2,3,4)
exponential.changes = [
	("genType", {1 :"const_float", 2 :"const_vec2", 3 :"const_vec3", 4 :"const_vec4"}),
]
exponential.protos = [
	"genType pow(genType x, genType y)",
	"genType exp(genType x)",
	"genType log(genType x)",
	"genType exp2(genType x)",
	"genType log2(genType x)",
	"genType sqrt(genType x)",
	"genType inversesqrt(genType x)",
]

common = proto_group()
common.name ="Common Functions"
common.change_dims = (1,2,3,4)
common.changes = [
	("genType", {1 :"const_float", 2 :"const_vec2", 3 :"const_vec3", 4 :"const_vec4"}),
]
common.protos = [
	"genType abs(genType x)",
	"genType sign(genType x)",
	"genType floor(genType x)",
	"genType ceil(genType x)",
	"genType fract(genType x)",
	"genType mod(genType x, const_float y)",
	"genType mod(genType x, genType y)",
	"genType min(genType x, genType y)",
	"genType min(genType x, const_float y)",
	"genType max(genType x, genType y)",
	"genType max(genType x, const_float y)",
	"genType clamp(genType x, genType minVal, genType maxVal)",
	"genType clamp(genType x, const_float minVal, const_float maxVal)",
	"genType mix(genType x, genType y, genType a)",
	"genType mix(genType x, genType y, const_float a)",
	"genType step(genType edge, genType x)",
	"genType step(const_float edge, genType x)",
	"genType smoothstep(genType edge0, genType edge1, genType x)",
	"genType smoothstep(const_float edge0, const_float edge1, genType x)",
]

geometric = proto_group()
geometric.name ="Geometric Functions"
geometric.change_dims = (1,2,3,4)
geometric.changes = [
	("genType", {1 :"const_float", 2 :"const_vec2", 3 :"const_vec3", 4 :"const_vec4"}),
]
geometric.protos = [
	"const_float length(genType x)",
	"const_float distance(genType p0, genType p1)",
	"const_float dot(genType x, genType y)",
	"const_vec3 cross(const_vec3 x, const_vec3 y)",
	"genType normalize(genType x)",
	"genType faceforward(genType N, genType I, genType Nref)",
	"genType reflect(genType I, genType N)",
	"genType refract(genType I, genType N, const_float eta)",
]

matrix = proto_group()
matrix.name ="Matrix Functions"
matrix.change_dims = (2,3,4)
matrix.changes = [
	("mat", {2 :"const_mat2", 3 :"const_mat3", 4 :"const_mat4"}),
]
matrix.protos = [
	"mat matrixCompMult(mat x, mat y)",
]

vector_relational = proto_group()
vector_relational.name ="Vector Relational Functions"
vector_relational.change_dims = (2,3,4)
vector_relational.changes = [
	("ivec", {2 :"const_ivec2", 3 :"const_ivec3", 4 :"const_ivec4"}),
	("bvec", {2 :"const_bvec2", 3 :"const_bvec3", 4 :"const_bvec4"}),
	("vec", {2 :"const_vec2", 3 :"const_vec3", 4 :"const_vec4"}),
]
vector_relational.protos = [
	"bvec lessThan(vec x, vec y)",
	"bvec lessThan(ivec x, ivec y)",
	"bvec lessThanEqual(vec x, vec y)",
	"bvec lessThanEqual(ivec x, ivec y)",
	"bvec greaterThan(vec x, vec y)",
	"bvec greaterThan(ivec x, ivec y)",
	"bvec greaterThanEqual(vec x, vec y)",
	"bvec greaterThanEqual(ivec x, ivec y)",
	"bvec equal(vec x, vec y)",
	"bvec equal(ivec x, ivec y)",
	"bvec equal(bvec x, bvec y)",
	"bvec notEqual(vec x, vec y)",
	"bvec notEqual(ivec x, ivec y)",
	"bvec notEqual(bvec x, bvec y)",
	"const_bool any(bvec x)",
	"const_bool all(bvec x)",
	"bvec not(bvec x)",
]

proto_groups = [
	angle_and_trig,
	exponential,
	common,
	geometric,
	matrix,
	vector_relational
]

# pre compile identifier regex for speed
regex_id = re.compile(r"(\w+)")
regex_id_const = re.compile(r"(const_\w+) ")

# function table
function_names = []
function_sigs = []
function_sigs_enum = []
function_overloads = {} # will be e.g. { "radians" : [radians__const_float__const_float, ...] }

# make definitions and populate function table
for proto_group in proto_groups:

	function_prototypes = []
	definitions = []

	for proto in proto_group.protos:

		# if there's a base (dim 1) function we'll insert component wise calls later
		base_function = "__BASEFN__"

		for dim in proto_group.change_dims:

			# new definition
			deflines = []
			proto_changed = proto

			# find return type
			return_type = regex_id.search(proto_changed).group()
			# replace return with void and add parameter (pointer star will be added later)
			proto_changed = proto_changed.replace(return_type, "void", 1).replace("(", "(" + return_type + " result, ")

			# make type substitutions of consistent dimension
			for change in proto_group.changes:
				proto_changed = proto_changed.replace(change[0] + " ", change[1][dim] + " ")

			# mangle types into function name
			i = 0
			old_name = ""
			mangled_name = ""
			sig = ""
			argnames = []
			for id in regex_id.finditer(proto_changed):
				if i == 1:
					# function name
					old_name = id.group(1)
					mangled_name = old_name
				elif i >= 2 and i % 2 == 0:
					# signature type
					sig += "__" + id.group(1)
				elif i >= 2 and i % 2 == 1:
					# arg name
					argnames.append(id.group(1))
				i+=1
			mangled_name += sig

			# set base name for component wise calls
			if dim == 1:
				base_function = mangled_name

			# replace name
			proto_changed = proto_changed.replace(old_name + "(", mangled_name + "(")

			# save as an overload
			if old_name not in function_overloads:
				function_overloads[old_name] = []
			if mangled_name not in function_overloads[old_name]:
				function_overloads[old_name].append(mangled_name)

			# pointer-ify all const_ stuff
			proto_changed = regex_id_const.sub(r"\1* ", proto_changed)

			# create definition
			deflines.append(proto_changed)

			deflines.append("{")
			deflines.append("\tassert(0); // todo")
			deflines.append("\tTRACE((\"in "+mangled_name+"()\\n\"));")

			if dim == 1:
				line = "__EXTERNFN__" + "("
				for argname in argnames:
					line += argname + ", "
				line = line[:len(line)-2]
				line += ");"
				deflines.append("\t//" + line)
			else:
				for i in range(dim):
					sub = "[" + str(i) + "]"
					line = base_function + "("
					for argname in argnames:
						line += "&(*" + argname + ")" + sub + ", "
					line = line[:len(line)-2]
					line += ");"
					deflines.append("\t//" + line)

			deflines.append("}")

			# done
			definition = "\n".join(deflines)

			# eliminate duplicates
			if proto_changed not in function_prototypes:
				function_names.append(mangled_name)
				function_prototypes.append(proto_changed)
				definitions.append(definition)
			if sig not in function_sigs:
				function_sigs.append(sig)
				function_sigs_enum.append("CF_SIG" + sig.upper())

	proto_group.function_prototypes = function_prototypes
	proto_group.definitions = definitions


# print code


# shader language header
if False:

	regex_id_result = re.compile(r"^(.*?)__.*?\((\w+) result,? ?(.*?)$")

	for proto_group in proto_groups:

		print "//"
		print "// "+ proto_group.name
		print "//"

		print

		for fp in proto_group.function_prototypes:

			# hack shader language prototypes from the existing C prototypes

			slang_sig = fp[len("void "):].replace("*", "").replace("const_", "")
			groups = regex_id_result.match(slang_sig).groups()
			slang_sig = groups[1] + " " + groups[0] + "(" + groups[2]
			print "// todo"
			print '"' + slang_sig + ';"'
			print

		print

	print
	print "##################################################################"
	print

# function header declarations
if False:

	for proto_group in proto_groups:

		print "// "+ proto_group.name

		for fp in proto_group.function_prototypes:
			print fp + ";"

		print

	print
	print "##################################################################"
	print

# signature enum
if False:

	print "typedef enum"
	print "{"

	for fse in function_sigs_enum:
		print "\t" + fse + ","

	print "\tCONST_FUNCTION_SIG_COUNT"
	print "} ConstFunctionSignature;"

	print
	print "##################################################################"
	print

# signature SymbolType init code
if False:

	regex_id_sig = re.compile(r"(CONST_[A-Z0-9]+)")

	for fs in function_sigs_enum:
		arg_count = -1
		argtypes = []
		return_type = ""

		for type in regex_id_sig.finditer(fs):
			if arg_count >= 0:
				argtypes.append(type.group(1).replace("CONST", "PRIM"))
			else:
				return_type = type.group(1).replace("CONST", "PRIM")
			arg_count += 1

		typename = "\t\tconstantFunctionSignatures["+fs+"].name = \"("
		for argtype in argtypes[:-1]:
			typename += argtype[len("PRIM_"):].lower() + ", "
		typename += argtypes[len(argtypes)-1][len("PRIM_"):].lower()
		print typename + ")\";"

		print "\t\tconstantFunctionSignatures["+fs+"].function_type.param_count = " + str(arg_count) + ";"
		print "\t\tconstantFunctionSignatures["+fs+"].function_type.params = (Symbol **)malloc_fast(sizeof(Symbol*) * " + str(arg_count) + ");"
		i = 0
		for argtype in argtypes:
			print "\t\tconstantFunctionSignatures["+fs+"].function_type.params["+str(i)+"] = &primitiveParamSymbols["+argtype+"];"
			i += 1
		print "\t\tconstantFunctionSignatures["+fs+"].function_type.return_type = &primitiveTypes["+return_type+"];"
		print

	print
	print "##################################################################"
	print

# function Symbol init code
if True:

	for root_name in function_overloads:

		# for all root names

		print "\t\tconst char *intern_" +root_name+ " = glsl_intern(\"" +root_name+ "\", false);"

	print

	for root_name in function_overloads:

		# for all root names

		last_index = None

		for mangled_name in function_overloads[root_name]:

			# for all overloads

			index = "CF__" + mangled_name.upper()
			sig = "CF_SIG" + mangled_name[mangled_name.find("__"):len(mangled_name)].upper()

			next_overload = None

			if last_index == None:
				next_overload = "NULL"
			else:
				# take great care to avoid cycles!!
				next_overload = "&constantFunctions["+last_index+"]"

			print "\t\tglsl_symbol_construct_function_instance("
			print "\t\t\t&constantFunctions["+index+"]," #address
			print "\t\t\tintern_" +root_name+ "," #name
			print "\t\t\t&constantFunctionSignatures["+sig+"]," #type
			print "\t\t\t(ConstantFunction)" +mangled_name+ "," #definition
			print "\t\t\t" +mangled_name+ "__body," #definition
			print "\t\t\t" +next_overload+ ");" #next overload

			print

			last_index = index

	print
	print "##################################################################"
	print

# function body names
if False:

	for root_name in function_overloads:

		# for all root names

		for mangled_name in function_overloads[root_name]:

			# for all overloads

			print "const char *" +mangled_name+ "__body = " #definition

	print
	print "##################################################################"
	print

# function enum
if False:

	print "typedef enum"
	print "{"

	for fn in function_names:
		print "\tCF__" + fn.upper() + ","

	print "\tCONST_FUNCTION_COUNT"
	print "} ConstFunction;"

	print
	print "##################################################################"
	print

# function definitions
if False:

	for proto_group in proto_groups:

		#print "// "+ proto_group.name
		#print

		for definition in proto_group.definitions:
			print definition

		#print
		print
