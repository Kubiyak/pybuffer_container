import os
# Set these based on env vars to pick up python 3.8.1 libs and includes

py_install_dir = os.environ.get("PY_INSTALL_DIR", "../../python/python-3.8.1")
if not os.path.exists(py_install_dir):
    raise RuntimeError("PY_INSTALL_DIR must be set to point to where python 3.8+ include/ lib/ dirs are")

Repository(py_install_dir + '/include')

github_checkout = Environment()
github_checkout.Command("snapshot_container/.git", None,
                                    "git clone https://github.com/Kubiyak/snapshot_container")
github_checkout.Command("magic_get/.git", None, "git clone https://github.com/apolukhin/magic_get")
github_checkout.Command("metal/.git", None, "git clone https://github.com/brunocodutra/metal")

Repository("snapshot_container")
Repository("magic_get/include")
Repository("metal/include")


# example_env = Environment(CXX="g++-8", CXXFLAGS="--std=c++17 -g", CPPPATH=["."])
# example = example_env.Program("example", ["python_struct.cpp"])


header_files = ['pybuffer_storage.h', 'pybuffer_container.h']


pybuffer_container_env = Environment(CXX="g++-8", CXXFLAGS="--std=c++17 -g", CPPPATH=["."])
pybuffer_container_test = pybuffer_container_env.Program("build/pybuffer_container_test/pybuffer_container_test", ["pybuffer_container_test.cpp"])
pybuffer_container_env.VariantDir("build/pybuffer_container_test", "./")
Depends('build/pybuffer_container_test/pybuffer_container_test', header_files + ['snapshot_container/', 'metal/', 'magic_get/'])
pybuffer_container_env.Alias('pybuffer_container_test', pybuffer_container_test)
