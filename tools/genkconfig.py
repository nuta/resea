#!/usr/bin/env python3
import argparse
import re
import jinja2
from glob import glob
from pathlib import Path


# If you're familiar with Linux kernel development, you may notice that this
# config generates `tristate` for each server.
#
# The `M` state is usually used for kernel modules, however, in Resea, a server
# with `M` will be embedded into the OS image but it's not automatically started
# (you need to start it from the shell).
TMPL = """\
config MODULES
    def_bool y
	option modules
	default y

choice
    prompt "Target CPU architecture"
{%- for arch in archs %}
    config ARCH_{{ arch.name | upper }}
        bool "{{ arch.name }}"
{%- endfor %}
endchoice

# A hidden config entry to extract the selected arch name in Makefile.
{%- for arch in archs %}
config ARCH
    string
    default "{{ arch.name }}"
    depends on ARCH_{{ arch.name | upper }}
{%- endfor %}

# Machines
{%- for arch in archs %}
{%- if arch.machines | length > 0 %}
if ARCH_{{ arch.name | upper }}
choice
    prompt "Target Machine"
{%- for machine in arch.machines %}
    config MACHINE_{{ machine.name | upper }}
        bool "{{ machine.name }}"
{%- endfor %}
endchoice
endif
{%- endif %}
{%- endfor %}

choice
    prompt "Initial task"
    default BOOT_TASK_VM
{%- for server in servers %}
{%- if server.boot_task %}
    config BOOT_TASK_{{ server.name | upper }}
        bool "{{ server.name }}"
{%- endif %}
{%- endfor %}
endchoice

menu "Servers"
    comment "<*>: autostarted / <M>: manually started from shell"

{%- for server in servers %}
{%- if server.boot_task %}
source "{{ server.kconfig_path }}"
{%- endif %}
{%- endfor %}

{%- for section_name,servers in server_sections.items() %}
{%- if section_name %}
    menu "{{ section_name }}"
{%- endif %}
    {%- for server in servers %}
        {%- if not server.boot_task %}
            config {{ server.name | upper }}_SERVER
                tristate "{{ server.name }}"
            {%- if server.kconfig_path %}
            source "{{ server.kconfig_path }}"
            {%- endif %}
        {%- endif %}
    {%- endfor %}
{%- if section_name %}
    endmenu
{%- endif %}
{%- endfor %}
endmenu

"""

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", dest="outfile", required=True)
    args = parser.parse_args()

    section_mappings = {
        "servers": None,
        "servers/fs": "File Systems",
        "servers/apps": "Applications",
        "servers/drivers/blk": "Block Drivers",
        "servers/drivers/net": "Network Drivers",
    }

    servers = []
    server_sections = {}
    for path in sorted(glob("servers/**/build.mk", recursive=True)):
        kconfig = open(path).read()
        d = dict(re.findall(r"(?P<key>[a-zA-Z0-9_\-]+).+=[ ]*(?P<value>.+)[ ]*", kconfig))
        kconfig_path = Path(path).parent / "Kconfig"
        if kconfig_path.exists():
            d["kconfig_path"] = kconfig_path
        section_dir = str(Path(path).parent.parent)
        if section_dir not in section_mappings:
            raise Exception(f"please update `section_mappings' for {path}")
        servers.append(d)
        section_name = section_mappings[section_dir]
        server_sections.setdefault(section_name, [])
        server_sections[section_name].append(d)

    archs = []
    for arch_dir in glob("kernel/arch/*"):
        arch_dir = Path(arch_dir)
        if arch_dir.is_dir():
            machines = []
            for machine_dir in glob((arch_dir / "machines" / "*").as_posix()):
                machine_dir = Path(machine_dir)
                if machine_dir.is_dir():
                    machines.append({ "name": machine_dir.name })
            archs.append({
                "name": arch_dir.name,
                "machines": machines,
            })

    with open(args.outfile, "w") as f:
        f.write(jinja2.Template(TMPL).render(**locals()))

if __name__ == "__main__":
    main()
