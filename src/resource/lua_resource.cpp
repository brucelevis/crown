/*
 * Copyright (c) 2012-2016 Daniele Bartolini and individual contributors.
 * License: https://github.com/taylor001/crown/blob/master/LICENSE
 */

#include "array.h"
#include "compile_options.h"
#include "config.h"
#include "dynamic_string.h"
#include "lua_resource.h"
#include "os.h"
#include "string_stream.h"
#include "temp_allocator.h"

#define LUAJIT_NAME "./luajit"

#if CROWN_PLATFORM_WINDOWS
	#define EXE ".exe"
#else
 	#define EXE ""
#endif // CROWN_PLATFORM_WINDOWS

#define LUAJIT_EXE LUAJIT_NAME EXE

#if CROWN_DEBUG
	#define LUAJIT_FLAGS "-bg" // Keep debug info
#else
	#define LUAJIT_FLAGS "-b"
#endif // CROWN_DEBUG

namespace crown
{
namespace lua_resource_internal
{
	void compile(const char* path, CompileOptions& opts)
	{
		TempAllocator1024 ta;
		DynamicString luasrc(ta);
		DynamicString luabin(ta);
		opts.get_absolute_path(path, luasrc);
		opts.get_temporary_path("lua.bin", luabin);

		StringStream args(ta);
		args << " " << LUAJIT_FLAGS;
		args << " " << luasrc.c_str();
		args << " " << luabin.c_str();

		StringStream output(ta);
		int exitcode = os::execute_process(LUAJIT_EXE, string_stream::c_str(args), output);
		RESOURCE_COMPILER_ASSERT(exitcode == 0
			, opts
			, "Failed to compile lua:\n%s"
			, string_stream::c_str(output)
			);

		Buffer blob = opts.read_temporary(luabin.c_str());
		opts.delete_file(luabin.c_str());

		LuaResource lr;
		lr.version = RESOURCE_VERSION_SCRIPT;
		lr.size = array::size(blob);

		opts.write(lr.version);
		opts.write(lr.size);
		opts.write(blob);
	}

	void* load(File& file, Allocator& a)
	{
		const u32 file_size = file.size();
		void* res = a.allocate(file_size);
		file.read(res, file_size);
		CE_ASSERT(*(u32*)res == RESOURCE_VERSION_SCRIPT, "Wrong version");
		return res;
	}

	void unload(Allocator& allocator, void* resource)
	{
		allocator.deallocate(resource);
	}
} // namespace lua_resource_internal

namespace lua_resource
{
	const char* program(const LuaResource* lr)
	{
		return (char*)&lr[1];
	}
} // namespace lua_resource

} // namespace crown
