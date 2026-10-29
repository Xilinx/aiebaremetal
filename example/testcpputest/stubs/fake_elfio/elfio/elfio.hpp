// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef ELFIO_HPP
#define ELFIO_HPP

#include <cstdint>
#include <cstring>
#include <fstream>
#include <istream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ELFIO {

using Elf_Half = uint16_t;
using Elf_Word = uint32_t;
using Elf_Xword = uint64_t;
using Elf_Sxword = int64_t;
using Elf64_Addr = uint64_t;
using Elf32_Addr = uint32_t;

struct Elf64_Sym {
	Elf_Word st_name;
	unsigned char st_info;
	unsigned char st_other;
	Elf_Half st_shndx;
	Elf64_Addr st_value;
	Elf_Xword st_size;
};

struct Elf32_Sym {
	Elf_Word st_name;
	Elf32_Addr st_value;
	Elf_Word st_size;
	unsigned char st_info;
	unsigned char st_other;
	Elf_Half st_shndx;
};

struct Elf64_Rela {
	Elf64_Addr r_offset;
	Elf_Xword r_info;
	Elf_Sxword r_addend;
};

constexpr unsigned char ELFCLASS32 = 1;
constexpr unsigned char ELFCLASS64 = 2;
constexpr unsigned char ELFDATA2LSB = 1;
constexpr unsigned char ELFDATA2MSB = 2;

constexpr Elf_Word ET_REL = 1;
constexpr Elf_Word EM_X86_64 = 62;

constexpr Elf_Word SHT_NULL = 0;
constexpr Elf_Word SHT_PROGBITS = 1;
constexpr Elf_Word SHT_STRTAB = 3;
constexpr Elf_Word SHT_RELA = 4;
constexpr Elf_Word SHT_NOBITS = 8;
constexpr Elf_Word SHT_DYNSYM = 11;

constexpr Elf_Word SHF_ALLOC = 0x2;
constexpr Elf_Word SHF_WRITE = 0x1;
constexpr Elf_Word SHF_EXECINSTR = 0x4;

constexpr Elf_Word PT_NULL = 0;

constexpr unsigned char STB_GLOBAL = 1;
constexpr unsigned char STT_OBJECT = 1;

constexpr unsigned char ELFOSABI_NONE = 0;

constexpr unsigned char ELFMAG0 = 0x7f;
constexpr unsigned char ELFMAG1 = 'E';
constexpr unsigned char ELFMAG2 = 'L';
constexpr unsigned char ELFMAG3 = 'F';

constexpr int EI_CLASS = 4;
constexpr int EI_DATA = 5;
constexpr int EI_ABIVERSION = 8;

inline unsigned char ELF64_ST_BIND(unsigned char info) { return static_cast<unsigned char>(info >> 4); }
inline unsigned char ELF64_ST_TYPE(unsigned char info) { return static_cast<unsigned char>(info & 0xfU); }
inline Elf_Xword ELF64_R_SYM(Elf_Xword info) { return info >> 32; }
inline Elf_Xword ELF64_R_TYPE(Elf_Xword info) { return info & 0xffffffffULL; }
inline Elf_Xword ELF64_R_INFO(Elf_Xword sym, Elf_Xword type) {
	return (sym << 32U) | (type & 0xffffffffULL);
}

inline Elf_Word ELF32_R_SYM(Elf_Word info) { return info >> 8U; }
inline Elf_Word ELF32_R_TYPE(Elf_Word info) { return info & 0xffU; }

class elfio;

class section {
	friend class elfio;
	friend class relocation_section_accessor;
	friend class symbol_section_accessor;

public:
	const std::string &get_name() const noexcept { return name_; }
	const char *get_data() const noexcept { return data_ptr_; }
	Elf_Half get_index() const noexcept { return index_; }
	Elf_Xword get_size() const noexcept { return size_; }
	Elf_Xword get_entry_size() const noexcept { return entry_size_; }
	Elf_Word get_type() const noexcept { return type_; }
	Elf_Word get_link() const noexcept { return link_; }

private:
	std::string name_;
	const char *data_ptr_{nullptr};
	std::vector<char> owned_data_;
	Elf_Half index_{0};
	Elf_Xword size_{0};
	Elf_Xword entry_size_{0};
	Elf_Word type_{SHT_NULL};
	Elf_Word link_{0};
};

class segment {
	friend class elfio;

public:
	Elf_Word get_type() const noexcept { return type_; }
	Elf_Word get_flags() const noexcept { return flags_; }
	Elf64_Addr get_virtual_address() const noexcept { return vaddr_; }
	Elf64_Addr get_physical_address() const noexcept { return paddr_; }
	Elf_Xword get_file_size() const noexcept { return filesz_; }
	Elf_Xword get_memory_size() const noexcept { return memsz_; }
	Elf_Xword get_align() const noexcept { return align_; }

private:
	Elf_Word type_{PT_NULL};
	Elf_Word flags_{0};
	Elf64_Addr vaddr_{0};
	Elf64_Addr paddr_{0};
	Elf_Xword filesz_{0};
	Elf_Xword memsz_{0};
	Elf_Xword align_{0};
};

inline uint16_t ru16(const unsigned char *p) {
	return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}
inline uint32_t ru32(const unsigned char *p) {
	return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
	       (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
inline uint64_t ru64(const unsigned char *p) {
	uint64_t lo = ru32(p);
	uint64_t hi = ru32(p + 4);
	return lo | (hi << 32);
}

inline uint16_t ru16_ex(unsigned char enc, const unsigned char *p) {
	return enc != ELFDATA2MSB ? ru16(p)
				 : static_cast<uint16_t>((static_cast<uint32_t>(ru16(p)) << 8U) | (ru16(p) >> 8U));
}
inline uint32_t ru32_ex(unsigned char enc, const unsigned char *p) {
	return enc != ELFDATA2MSB ? ru32(p) : __builtin_bswap32(ru32(p));
}
inline uint64_t ru64_ex(unsigned char enc, const unsigned char *p) {
	return enc != ELFDATA2MSB ? ru64(p) : __builtin_bswap64(ru64(p));
}

struct elfio_ut_ShdrRow {
	Elf_Word name_off{};
	Elf_Word type{};
	Elf_Xword flags{};
	Elf_Xword addr{};
	Elf_Xword offset{};
	Elf_Xword size{};
	Elf_Word link{};
	Elf_Word info{};
	Elf_Xword addralign{};
	Elf_Xword entsize{};
};

inline elfio_ut_ShdrRow read_elf_shdr(bool is64, unsigned char enc, const unsigned char *sh) {
	elfio_ut_ShdrRow r;
	r.name_off = ru32_ex(enc, sh + 0);
	r.type = ru32_ex(enc, sh + 4);
	if (is64) {
		r.flags = ru64_ex(enc, sh + 8);
		r.addr = ru64_ex(enc, sh + 16);
		r.offset = ru64_ex(enc, sh + 24);
		r.size = ru64_ex(enc, sh + 32);
		r.link = ru32_ex(enc, sh + 40);
		r.info = ru32_ex(enc, sh + 44);
		r.addralign = ru64_ex(enc, sh + 48);
		r.entsize = ru64_ex(enc, sh + 56);
	} else {
		r.flags = ru32_ex(enc, sh + 8);
		r.addr = ru32_ex(enc, sh + 12);
		r.offset = ru32_ex(enc, sh + 16);
		r.size = ru32_ex(enc, sh + 20);
		r.link = ru32_ex(enc, sh + 24);
		r.info = ru32_ex(enc, sh + 28);
		r.addralign = ru32_ex(enc, sh + 32);
		r.entsize = ru32_ex(enc, sh + 36);
	}
	return r;
}

class relocation_section_accessor {
public:
	relocation_section_accessor(elfio &file, section *sec) noexcept;
	Elf_Xword get_entries_num() const noexcept;
	bool get_entry(Elf_Xword index, Elf64_Addr &offset, Elf_Word &symbol, unsigned &type,
		       Elf_Sxword &addend) const noexcept;

private:
	elfio &elf_file_;
	section *sec_;
};

class symbol_section_accessor {
public:
	symbol_section_accessor(elfio &file, section *sec) noexcept;
	Elf_Xword get_symbols_num() const noexcept;
	bool get_symbol(Elf_Xword index, std::string &name, Elf64_Addr &value, Elf_Xword &size,
			unsigned char &bind, unsigned char &type, Elf_Half &section_index,
			unsigned char &other) const noexcept;

private:
	elfio &elf_file_;
	section *sec_;
};

class elfio {
public:
	class Sections {
		elfio *parent_;

	public:
		explicit Sections(elfio *p) noexcept : parent_(p) {}
		size_t size() const noexcept { return parent_->sections_.size(); }
		section *operator[](size_t i) noexcept {
			return i < parent_->sections_.size() ? parent_->sections_[i].get() : nullptr;
		}
	};

	class Segments {
		elfio *parent_;

	public:
		explicit Segments(elfio *p) noexcept : parent_(p) {}
		size_t size() const noexcept { return parent_->segments_.size(); }
		segment *operator[](size_t i) noexcept {
			return i < parent_->segments_.size() ? parent_->segments_[i].get() : nullptr;
		}
	};

	Sections sections;
	Segments segments;

	elfio() noexcept : sections(this), segments(this) {}

	bool load(const std::string &path) {
		std::ifstream f(path, std::ios::binary);
		return static_cast<bool>(f) && load(static_cast<std::istream &>(f));
	}

	bool load(std::istream &stream) {
		stream.seekg(0, std::ios::beg);
		image_.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
		return parse_image();
	}

	unsigned char get_class() const noexcept { return class_; }
	unsigned char get_encoding() const noexcept { return encoding_; }
	Elf_Half get_section_name_str_index() const noexcept { return shstrndx_; }
	uint8_t get_abi_version() const noexcept { return abi_version_; }

private:
	std::vector<char> image_;
	std::vector<std::unique_ptr<section>> sections_;
	std::vector<std::unique_ptr<segment>> segments_;
	unsigned char class_{ELFCLASS64};
	unsigned char encoding_{ELFDATA2LSB};
	uint8_t abi_version_{0};
	Elf_Half shstrndx_{0};

	bool install_sections_from_rows(const unsigned char *base, size_t n, Elf_Half shnum,
					  const std::vector<elfio_ut_ShdrRow> &rows);

	bool parse_elf_unified(bool is64, const unsigned char *base, size_t n);
	bool parse_elf64(const unsigned char *base, size_t n) { return parse_elf_unified(true, base, n); }
	bool parse_elf32(const unsigned char *base, size_t n) { return parse_elf_unified(false, base, n); }

	bool parse_image() {
		sections_.clear();
		segments_.clear();
		const unsigned char *base = reinterpret_cast<const unsigned char *>(image_.data());
		const size_t n = image_.size();
		if (n < 16U) {
			return false;
		}
		if (base[0] != ELFMAG0 || base[1] != ELFMAG1 || base[2] != ELFMAG2 || base[3] != ELFMAG3) {
			return false;
		}
		class_ = base[EI_CLASS];
		encoding_ = base[EI_DATA];
		if (encoding_ != ELFDATA2LSB && encoding_ != ELFDATA2MSB) {
			return false;
		}
		abi_version_ = base[EI_ABIVERSION];

		if (class_ == ELFCLASS64) {
			return parse_elf64(base, n);
		}
		if (class_ == ELFCLASS32) {
			return parse_elf32(base, n);
		}
		return false;
	}
};

inline bool elfio::parse_elf_unified(bool is64, const unsigned char *base, size_t n) {
	if (n < (is64 ? 64U : 52U)) {
		return false;
	}
	const unsigned char enc = encoding_;
	const uint64_t phoff = is64 ? ru64_ex(enc, base + 32) : ru32_ex(enc, base + 28);
	const uint64_t shoff = is64 ? ru64_ex(enc, base + 40) : ru32_ex(enc, base + 32);
	const Elf_Half phentsize = is64 ? ru16_ex(enc, base + 54) : ru16_ex(enc, base + 42);
	const Elf_Half phnum = is64 ? ru16_ex(enc, base + 56) : ru16_ex(enc, base + 44);
	const Elf_Half shentsize = is64 ? ru16_ex(enc, base + 58) : ru16_ex(enc, base + 46);
	const Elf_Half shnum = is64 ? ru16_ex(enc, base + 60) : ru16_ex(enc, base + 48);
	shstrndx_ = is64 ? ru16_ex(enc, base + 62) : ru16_ex(enc, base + 50);

	const size_t ph_need = is64 ? 56U : 32U;
	for (Elf_Half pi = 0; pi < phnum; ++pi) {
		const size_t off = static_cast<size_t>(phoff) + static_cast<size_t>(pi) * static_cast<size_t>(phentsize);
		if (off + ph_need > n || phentsize < ph_need) {
			continue;
		}
		const unsigned char *ph = base + off;
		auto seg = std::make_unique<segment>();
		seg->type_ = ru32_ex(enc, ph + 0);
		if (is64) {
			seg->flags_ = ru32_ex(enc, ph + 4);
			seg->vaddr_ = ru64_ex(enc, ph + 16);
			seg->paddr_ = ru64_ex(enc, ph + 24);
			seg->filesz_ = ru64_ex(enc, ph + 32);
			seg->memsz_ = ru64_ex(enc, ph + 40);
			seg->align_ = ru64_ex(enc, ph + 48);
		} else {
			seg->vaddr_ = ru32_ex(enc, ph + 8);
			seg->paddr_ = ru32_ex(enc, ph + 12);
			seg->filesz_ = ru32_ex(enc, ph + 16);
			seg->memsz_ = ru32_ex(enc, ph + 20);
			seg->flags_ = ru32_ex(enc, ph + 24);
			seg->align_ = ru32_ex(enc, ph + 28);
		}
		segments_.push_back(std::move(seg));
	}

	const Elf_Half sh_minsz = is64 ? 64 : 40;
	if (shoff == 0U || shnum == 0 || shentsize < sh_minsz) {
		return true;
	}

	const size_t sh_read = is64 ? 64U : 40U;
	std::vector<elfio_ut_ShdrRow> rows(static_cast<size_t>(shnum));
	for (Elf_Half si = 0; si < shnum; ++si) {
		const size_t off = static_cast<size_t>(shoff) + static_cast<size_t>(si) * static_cast<size_t>(shentsize);
		if (off + sh_read > n) {
			return false;
		}
		rows[static_cast<size_t>(si)] = read_elf_shdr(is64, enc, base + off);
	}
	return install_sections_from_rows(base, n, shnum, rows);
}

inline bool elfio::install_sections_from_rows(const unsigned char *base, size_t n, Elf_Half shnum,
					      const std::vector<elfio_ut_ShdrRow> &rows) {
	const Elf_Half si_str = shstrndx_;
	if (si_str >= shnum || rows[static_cast<size_t>(si_str)].type != SHT_STRTAB) {
		return false;
	}
	const elfio_ut_ShdrRow &shstr_raw = rows[static_cast<size_t>(si_str)];
	const size_t str_off = static_cast<size_t>(shstr_raw.offset);
	const size_t str_sz = static_cast<size_t>(shstr_raw.size);
	if (str_off + str_sz > n) {
		return false;
	}
	const char *strtab = reinterpret_cast<const char *>(base + str_off);

	for (Elf_Half si = 0; si < shnum; ++si) {
		auto sec = std::make_unique<section>();
		const elfio_ut_ShdrRow &r = rows[static_cast<size_t>(si)];
		sec->index_ = si;
		sec->type_ = r.type;
		sec->link_ = r.link;
		sec->entry_size_ = r.entsize;
		sec->size_ = r.size;
		if (r.name_off < str_sz) {
			sec->name_.assign(strtab + r.name_off);
		}
		if (r.type == SHT_NOBITS) {
			sec->data_ptr_ = nullptr;
		} else if (r.size > 0U) {
			const size_t o = static_cast<size_t>(r.offset);
			if (o + static_cast<size_t>(r.size) > n) {
				return false;
			}
			sec->owned_data_.assign(base + o, base + o + static_cast<size_t>(r.size));
			sec->data_ptr_ = sec->owned_data_.data();
		} else {
			sec->data_ptr_ = "";
		}
		sections_.push_back(std::move(sec));
	}
	return true;
}

inline relocation_section_accessor::relocation_section_accessor(elfio &file, section *sec) noexcept
    : elf_file_(file), sec_(sec) {}

inline Elf_Xword relocation_section_accessor::get_entries_num() const noexcept {
	if (!sec_ || sec_->get_entry_size() == 0) {
		return 0;
	}
	return sec_->get_size() / sec_->get_entry_size();
}

inline bool relocation_section_accessor::get_entry(Elf_Xword index, Elf64_Addr &offset, Elf_Word &symbol,
						     unsigned &type, Elf_Sxword &addend) const noexcept {
	if (index >= get_entries_num() || !sec_ || sec_->get_type() != SHT_RELA) {
		return false;
	}
	const unsigned char *p =
	    reinterpret_cast<const unsigned char *>(sec_->get_data()) + static_cast<size_t>(index * sec_->get_entry_size());
	const unsigned char enc = elf_file_.get_encoding();
	if (elf_file_.get_class() == ELFCLASS64) {
		if (sec_->get_entry_size() < sizeof(Elf64_Rela)) {
			return false;
		}
		if (enc == ELFDATA2LSB) {
			const Elf64_Rela *const re = reinterpret_cast<const Elf64_Rela *>(p);
			offset = re->r_offset;
			symbol = static_cast<Elf_Word>(ELF64_R_SYM(re->r_info));
			type = static_cast<unsigned>(ELF64_R_TYPE(re->r_info));
			addend = re->r_addend;
			return true;
		}
		offset = static_cast<Elf64_Addr>(ru64_ex(enc, p + 0));
		const Elf_Xword info = ru64_ex(enc, p + 8);
		symbol = static_cast<Elf_Word>(ELF64_R_SYM(info));
		type = static_cast<unsigned>(ELF64_R_TYPE(info));
		addend = static_cast<Elf_Sxword>(ru64_ex(enc, p + 16));
		return true;
	}
	if (sec_->get_entry_size() < 12U) {
		return false;
	}
	offset = ru32_ex(enc, p + 0);
	const Elf_Word info = ru32_ex(enc, p + 4);
	symbol = ELF32_R_SYM(info);
	type = ELF32_R_TYPE(info);
	addend = static_cast<Elf_Sxword>(ru32_ex(enc, p + 8));
	return true;
}

inline symbol_section_accessor::symbol_section_accessor(elfio &file, section *sec) noexcept
    : elf_file_(file), sec_(sec) {}

inline Elf_Xword symbol_section_accessor::get_symbols_num() const noexcept {
	if (!sec_) {
		return 0;
	}
	const Elf_Xword es = sec_->get_entry_size();
	const Elf_Xword sz = sec_->get_size();
	const unsigned char cls = elf_file_.get_class();
	const size_t min_es =
	    (cls == ELFCLASS64) ? sizeof(Elf64_Sym) : (cls == ELFCLASS32) ? sizeof(Elf32_Sym) : 0U;
	if (min_es == 0U || es < min_es) {
		return 0;
	}
	return sz / es;
}

inline bool symbol_section_accessor::get_symbol(Elf_Xword index, std::string &name, Elf64_Addr &value,
						Elf_Xword &size, unsigned char &bind, unsigned char &type,
						Elf_Half &section_index, unsigned char &other) const noexcept {
	if (index >= get_symbols_num() || !sec_) {
		return false;
	}
	const Elf_Word link = sec_->get_link();
	if (link >= elf_file_.sections.size()) {
		return false;
	}
	const section *strsec = elf_file_.sections[static_cast<size_t>(link)];
	if (!strsec || strsec->get_type() != SHT_STRTAB || !strsec->get_data()) {
		return false;
	}
	const char *strtab = strsec->get_data();
	const size_t str_sz = static_cast<size_t>(strsec->get_size());
	const unsigned char *p =
	    reinterpret_cast<const unsigned char *>(sec_->get_data()) + static_cast<size_t>(index * sec_->get_entry_size());
	const unsigned char enc = elf_file_.get_encoding();
	Elf_Word name_off = 0;
	unsigned char info = 0;

	if (elf_file_.get_class() == ELFCLASS64) {
		if (sec_->get_entry_size() < sizeof(Elf64_Sym)) {
			return false;
		}
		if (enc == ELFDATA2LSB) {
			const Elf64_Sym *const s = reinterpret_cast<const Elf64_Sym *>(p);
			name_off = s->st_name;
			info = s->st_info;
			other = s->st_other;
			section_index = s->st_shndx;
			value = s->st_value;
			size = s->st_size;
		} else {
			name_off = ru32_ex(enc, p + 0);
			info = p[4];
			other = p[5];
			section_index = ru16_ex(enc, p + 6);
			value = ru64_ex(enc, p + 8);
			size = ru64_ex(enc, p + 16);
		}
	} else if (elf_file_.get_class() == ELFCLASS32) {
		if (sec_->get_entry_size() < sizeof(Elf32_Sym)) {
			return false;
		}
		name_off = ru32_ex(enc, p + 0);
		value = ru32_ex(enc, p + 4);
		size = ru32_ex(enc, p + 8);
		info = p[12];
		other = p[13];
		section_index = ru16_ex(enc, p + 14);
	} else {
		return false;
	}
	bind = ELF64_ST_BIND(info);
	type = ELF64_ST_TYPE(info);
	if (name_off < str_sz) {
		name.assign(strtab + name_off);
	} else {
		name.clear();
	}
	return true;
}

inline void ut_set_elf_ident(unsigned char *e, unsigned char cls, unsigned char enc, uint8_t abi_ver) {
	e[0] = ELFMAG0;
	e[1] = ELFMAG1;
	e[2] = ELFMAG2;
	e[3] = ELFMAG3;
	e[EI_CLASS] = cls;
	e[EI_DATA] = enc;
	e[6] = 1;
	e[7] = ELFOSABI_NONE;
	e[EI_ABIVERSION] = abi_ver;
}

namespace cert_elf_ut_blob_detail {

inline constexpr Elf_Half kShNum = 10;
inline constexpr Elf_Half kShStrNdx = 1;

inline constexpr uint32_t kShName[10] = {0, 1, 11, 19, 27, 37, 51, 65, 72, 79};

inline const char kShstrtab[] =
    "\0.shstrtab\0.dynstr\0.dynsym\0.rela.dyn\0.ctrltext.0.0\0.ctrldata.0.0\0.pad.0\0.other\0.nobits\0";

inline void ut_pad8(std::vector<char> &v) {
	while ((v.size() % 8U) != 0U) {
		v.push_back('\0');
	}
}
inline void ut_append_fill(std::vector<char> &b, size_t n, int c) {
	const size_t o = b.size();
	b.resize(o + n);
	std::memset(b.data() + static_cast<std::ptrdiff_t>(o), c, n);
}

template <typename T>
inline void ut_store_le(unsigned char *d, T v_in) {
	using U = std::make_unsigned_t<T>;
	U v = static_cast<U>(v_in);
	for (size_t i = 0; i < sizeof(T); ++i) {
		d[i] = static_cast<unsigned char>(v & 0xffU);
		v = static_cast<U>(v >> 8U);
	}
}
template <typename T>
inline void ut_append_le(std::vector<char> &b, T v) {
	const size_t o = b.size();
	b.resize(o + sizeof(T));
	ut_store_le(reinterpret_cast<unsigned char *>(b.data() + static_cast<std::ptrdiff_t>(o)), v);
}
template <typename T>
inline void ut_patch_le(std::vector<char> &b, size_t pos, T v) {
	ut_store_le(reinterpret_cast<unsigned char *>(b.data() + static_cast<std::ptrdiff_t>(pos)), v);
}

inline size_t ut_align8(size_t x) { return (x + 7U) & ~size_t{7U}; }

inline void ut_append_elf64_sym(std::vector<char> &b, uint32_t st_name, unsigned char st_info,
				unsigned char st_other, Elf_Half st_shndx, uint64_t st_value, uint64_t st_size) {
	ut_append_le(b, st_name);
	b.push_back(static_cast<char>(st_info));
	b.push_back(static_cast<char>(st_other));
	ut_append_le(b, st_shndx);
	ut_append_le(b, st_value);
	ut_append_le(b, st_size);
}

inline void ut_append_elf64_rela(std::vector<char> &b, uint64_t r_offset, uint64_t r_sym, uint64_t r_type,
				 int64_t r_addend) {
	ut_append_le(b, r_offset);
	ut_append_le(b, ELF64_R_INFO(r_sym, r_type));
	ut_append_le(b, static_cast<uint64_t>(r_addend));
}

inline void ut_append_shdr64(std::vector<char> &b, uint32_t sh_name, uint32_t sh_type, uint64_t sh_flags,
			     uint64_t sh_addr, uint64_t sh_offset, uint64_t sh_size, uint32_t sh_link,
			     uint32_t sh_info, uint64_t sh_addralign, uint64_t sh_entsize) {
	ut_append_le(b, sh_name);
	ut_append_le(b, sh_type);
	ut_append_le(b, sh_flags);
	ut_append_le(b, sh_addr);
	ut_append_le(b, sh_offset);
	ut_append_le(b, sh_size);
	ut_append_le(b, sh_link);
	ut_append_le(b, sh_info);
	ut_append_le(b, sh_addralign);
	ut_append_le(b, sh_entsize);
}

inline void ut_write_elf64_ehdr(std::vector<char> &b, uint64_t shoff, uint8_t abi_ver) {
	if (b.size() < 64U) {
		b.resize(64U);
	}
	unsigned char *const e = reinterpret_cast<unsigned char *>(b.data());
	ut_set_elf_ident(e, ELFCLASS64, ELFDATA2LSB, abi_ver);
	std::memset(e + 9, 0, 7);
	ut_patch_le(b, 16, static_cast<uint16_t>(ET_REL));
	ut_patch_le(b, 18, static_cast<uint16_t>(EM_X86_64));
	ut_patch_le(b, 20, static_cast<uint32_t>(1));
	ut_patch_le(b, 24, static_cast<uint64_t>(0));
	ut_patch_le(b, 32, static_cast<uint64_t>(0));
	ut_patch_le(b, 40, shoff);
	ut_patch_le(b, 48, static_cast<uint32_t>(0));
	ut_patch_le(b, 52, static_cast<uint16_t>(64));
	ut_patch_le(b, 54, static_cast<uint16_t>(56));
	ut_patch_le(b, 56, static_cast<uint16_t>(0));
	ut_patch_le(b, 58, static_cast<uint16_t>(64));
	ut_patch_le(b, 60, kShNum);
	ut_patch_le(b, 62, kShStrNdx);
}

}

struct utest_cert_rela_spec {
	uint64_t offset{};
	uint32_t sym{};
	uint32_t type{};
	int64_t addend{};
};

inline std::vector<char> utest_build_cert_elf64_common(uint8_t abi_ver, std::string_view dynstr_blob,
						       unsigned dynsym_count,
						       const std::vector<utest_cert_rela_spec> &relas) {
	using namespace cert_elf_ut_blob_detail;
	std::vector<char> b;
	b.resize(64U);
	b.insert(b.end(), kShstrtab, kShstrtab + sizeof(kShstrtab) - 1);
	ut_pad8(b);

	const size_t off_dynstr = b.size();
	b.insert(b.end(), dynstr_blob.begin(), dynstr_blob.end());
	ut_pad8(b);

	const size_t off_dynsym = b.size();
	static constexpr unsigned char kSymInfo = static_cast<unsigned char>((STB_GLOBAL << 4U) | STT_OBJECT);
	static constexpr uint32_t kSymNameOff[] = {0, 1, 4, 7, 10, 13, 16};
	for (unsigned si = 0; si < dynsym_count; ++si) {
		const uint32_t noff = (si < sizeof(kSymNameOff) / sizeof(kSymNameOff[0])) ? kSymNameOff[si] : 0U;
		const Elf_Half shndx = (si == 0U) ? 0 : 5;
		ut_append_elf64_sym(b, noff, (si == 0U) ? 0U : kSymInfo, 0, shndx, 0, 0);
	}
	ut_pad8(b);

	const size_t off_rela = b.size();
	for (const utest_cert_rela_spec &r : relas) {
		ut_append_elf64_rela(b, r.offset, r.sym, r.type, r.addend);
	}
	ut_pad8(b);

	const size_t off_text = b.size();
	static constexpr size_t kTextSz = 2048U;
	ut_append_fill(b, kTextSz, 0);

	const size_t off_data = b.size();
	static constexpr size_t kDataSz = 32U;
	ut_append_fill(b, kDataSz, 0xcc);

	const size_t off_padsec = b.size();
	static constexpr size_t kPadSz = 8U;
	ut_append_fill(b, kPadSz, 0x55);

	const size_t off_other = b.size();
	b.push_back(static_cast<char>(0xee));

	const size_t shoff = ut_align8(b.size());
	b.resize(shoff, '\0');

	const uint64_t dynsym_bytes = static_cast<uint64_t>(24U) * static_cast<uint64_t>(dynsym_count);
	const uint64_t rela_bytes = static_cast<uint64_t>(24U) * static_cast<uint64_t>(relas.size());
	struct ShRow {
		Elf_Half sec_ix;
		uint32_t type;
		uint64_t flags;
		uint64_t off;
		uint64_t sz;
		uint32_t link;
		uint32_t info;
		uint64_t align;
		uint64_t esz;
	};
	const ShRow shrows[] = {
	    {0, SHT_NULL, 0, 0, 0, 0, 0, 0, 0},
	    {1, SHT_STRTAB, 0, 64U, sizeof(kShstrtab) - 1, 0, 0, 1, 0},
	    {2, SHT_STRTAB, 0, static_cast<uint64_t>(off_dynstr), static_cast<uint64_t>(dynstr_blob.size()), 0, 0, 1, 0},
	    {3, SHT_DYNSYM, 0, static_cast<uint64_t>(off_dynsym), dynsym_bytes, 2, 1, 8, 24},
	    {4, SHT_RELA, 0, static_cast<uint64_t>(off_rela), rela_bytes, 3, 0, 8, 24},
	    {5, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, static_cast<uint64_t>(off_text), static_cast<uint64_t>(kTextSz), 0,
	     0, 16, 0},
	    {6, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE, static_cast<uint64_t>(off_data), static_cast<uint64_t>(kDataSz), 0, 0,
	     4, 0},
	    {7, SHT_PROGBITS, SHF_ALLOC, static_cast<uint64_t>(off_padsec), static_cast<uint64_t>(kPadSz), 0, 0, 1, 0},
	    {8, SHT_PROGBITS, SHF_ALLOC, static_cast<uint64_t>(off_other), 1, 0, 0, 1, 0},
	    {9, SHT_NOBITS, SHF_ALLOC | SHF_WRITE, 0, 128, 0, 0, 16, 0},
	};
	for (const ShRow &r : shrows) {
		ut_append_shdr64(b, kShName[r.sec_ix], r.type, r.flags, 0, r.off, r.sz, r.link, r.info, r.align, r.esz);
	}

	ut_write_elf64_ehdr(b, shoff, abi_ver);
	return b;
}

inline std::map<std::string_view, uint64_t> utest_default_reloc_symbol_map() {
	return std::map<std::string_view, uint64_t>{{"s2", 0x1000}, {"s3", 0x2000}, {"s6", 0x3000}, {"s7", 0x4000},
						    {"s9", 0x5000}, {"sbad", 0x6000}};
}

inline std::vector<char> utest_build_elf64_be_minimal(uint8_t abi_ver) {
	std::vector<char> b(64, '\0');
	unsigned char *const e = reinterpret_cast<unsigned char *>(b.data());
	ut_set_elf_ident(e, ELFCLASS64, ELFDATA2MSB, abi_ver);
	e[16] = 0;
	e[17] = 1;
	e[18] = 0;
	e[19] = 62;
	e[23] = 1;
	e[52] = 0;
	e[53] = 64;
	return b;
}

inline std::vector<char> utest_build_elf32_le_minimal(uint8_t abi_ver) {
	std::vector<char> b(52, '\0');
	unsigned char *const e = reinterpret_cast<unsigned char *>(b.data());
	ut_set_elf_ident(e, ELFCLASS32, ELFDATA2LSB, abi_ver);
	e[16] = 1;
	e[17] = 0;
	e[18] = 3;
	e[19] = 0;
	e[20] = 1;
	e[40] = 52;
	e[41] = 0;
	return b;
}

}

#endif
