// Minimization: no <filesystem> at all. A namespace-qualified class whose
// name is shadowed by a captured variable, functionally cast inside an if
// condition inside a [=] lambda. If this crashes, the bug is a generic
// front-end condition-parsing defect with no std::filesystem involvement.

namespace agi {
class path {
public:
	path(const char *) {}
	bool empty() const { return true; }
};
}

const char *get_mode(const char *path) {
	return [=]() -> const char * {
		if (agi::path(path).empty())
			return nullptr;
		return "other";
	}();
}
