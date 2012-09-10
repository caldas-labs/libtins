/*
 * libtins is a net packet wrapper library for crafting and
 * interpreting sniffed packets.
 *
 * Copyright (C) 2011 Nasel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdexcept>
#include <cassert>
#include "endianness.h"
#include "dhcp.h"
#include "ethernetII.h"

using std::string;
using std::list;
using std::runtime_error;

namespace Tins {
// Magic cookie: uint32_t. 
DHCP::DHCP() : _size(sizeof(uint32_t)) {
    opcode(BOOTREQUEST);
    htype(1); //ethernet
    hlen(EthernetII::address_type::address_size);
}

DHCP::DHCP(const uint8_t *buffer, uint32_t total_sz) 
: BootP(buffer, total_sz, 0), _size(sizeof(uint32_t))
{
    buffer += BootP::header_size() - vend().size();
    total_sz -= BootP::header_size() - vend().size();
    uint8_t args[2] = {0};
    if(total_sz < sizeof(uint32_t) || *(uint32_t*)buffer != Endian::host_to_be<uint32_t>(0x63825363))
        throw std::runtime_error("Not enough size for a DHCP header in the buffer.");
    buffer += sizeof(uint32_t);
    total_sz -= sizeof(uint32_t);
    while(total_sz) {
        for(unsigned i(0); i < 2; ++i) {
            args[i] = *(buffer++);
            total_sz--;
            if(args[0] == END || args[0] == PAD) {
                args[1] = 0;
                i = 2;
            }
            else if(!total_sz)
                throw std::runtime_error("Not enough size for a DHCP header in the buffer.");
        }
        if(total_sz < args[1])
            throw std::runtime_error("Not enough size for a DHCP header in the buffer.");
        add_option(dhcp_option((Options)args[0], args[1], buffer));
        buffer += args[1];
        total_sz -= args[1];
    }
}

void DHCP::add_option(const dhcp_option &option) {
    _options.push_back(option);
    _size += option.data_size() + (sizeof(uint8_t) << 1);
}

const DHCP::dhcp_option *DHCP::search_option(Options opt) const {
    for(options_type::const_iterator it = _options.begin(); it != _options.end(); ++it) {
        if(it->option() == opt)
            return &(*it);
    }
    return 0;
}

void DHCP::type(Flags type) {
    uint8_t int_type = type;
    add_option(dhcp_option(DHCP_MESSAGE_TYPE, sizeof(uint8_t), &int_type));
}

void DHCP::end() {
    add_option(dhcp_option(DHCP_MESSAGE_TYPE));
}

uint8_t DHCP::type() const {
    return generic_search(DHCP_MESSAGE_TYPE, type2type<uint8_t>());
}

void DHCP::server_identifier(ipaddress_type ip) {
    uint32_t ip_int = ip;
    add_option(dhcp_option(DHCP_SERVER_IDENTIFIER, sizeof(uint32_t), (const uint8_t*)&ip_int));
}

DHCP::ipaddress_type DHCP::server_identifier() const {
    return generic_search(DHCP_SERVER_IDENTIFIER, type2type<ipaddress_type>());
}

void DHCP::lease_time(uint32_t time) {
    time = Endian::host_to_be(time);
    add_option(dhcp_option(DHCP_LEASE_TIME, sizeof(uint32_t), (const uint8_t*)&time));
}

uint32_t DHCP::lease_time() const {
    return Endian::host_to_be(generic_search(DHCP_LEASE_TIME, type2type<uint32_t>()));
}

void DHCP::renewal_time(uint32_t time) {
    time = Endian::host_to_be(time);
    add_option(dhcp_option(DHCP_RENEWAL_TIME, sizeof(uint32_t), (const uint8_t*)&time));
}
        
uint32_t DHCP::renewal_time() const {
    return Endian::host_to_be(generic_search(DHCP_RENEWAL_TIME, type2type<uint32_t>()));
}

void DHCP::subnet_mask(ipaddress_type mask) {
    uint32_t mask_int = mask;
    add_option(dhcp_option(SUBNET_MASK, sizeof(uint32_t), (const uint8_t*)&mask_int));
}

DHCP::ipaddress_type DHCP::subnet_mask() const {
    return generic_search(SUBNET_MASK, type2type<ipaddress_type>());
}

void DHCP::routers(const list<ipaddress_type> &routers) {
    serialization_type buffer = serialize_list(routers);
    add_option(dhcp_option(ROUTERS, buffer.begin(), buffer.end()));
}

std::list<DHCP::ipaddress_type> DHCP::routers() const {
    return generic_search(ROUTERS, type2type<std::list<ipaddress_type> >());
}

void DHCP::domain_name_servers(const list<ipaddress_type> &dns) {
    serialization_type buffer = serialize_list(dns);
    add_option(dhcp_option(DOMAIN_NAME_SERVERS, buffer.begin(), buffer.end()));
}

std::list<DHCP::ipaddress_type> DHCP::domain_name_servers() const {
    return generic_search(DOMAIN_NAME_SERVERS, type2type<std::list<ipaddress_type> >());
}

void DHCP::broadcast(ipaddress_type addr) {
    uint32_t int_addr = addr;
    add_option(dhcp_option(BROADCAST_ADDRESS, sizeof(uint32_t), (uint8_t*)&int_addr));
}

DHCP::ipaddress_type DHCP::broadcast() const {
    return generic_search(BROADCAST_ADDRESS, type2type<ipaddress_type>());
}

void DHCP::requested_ip(ipaddress_type addr) {
    uint32_t int_addr = addr;
    add_option(dhcp_option(DHCP_REQUESTED_ADDRESS, sizeof(uint32_t), (uint8_t*)&int_addr));
}

DHCP::ipaddress_type DHCP::requested_ip() const {
    return generic_search(DHCP_REQUESTED_ADDRESS, type2type<ipaddress_type>());
}

void DHCP::domain_name(const string &name) {
    add_option(dhcp_option(DOMAIN_NAME, name.size(), (const uint8_t*)name.c_str()));
}

std::string DHCP::domain_name() const {
    return generic_search(DOMAIN_NAME, type2type<std::string>());
}

void DHCP::rebind_time(uint32_t time) {
    time = Endian::host_to_be(time);
    add_option(dhcp_option(DHCP_REBINDING_TIME, sizeof(uint32_t), (uint8_t*)&time));
}
        
uint32_t DHCP::rebind_time() const {
    return Endian::host_to_be(generic_search(DHCP_REBINDING_TIME, type2type<uint32_t>()));
}

PDU::serialization_type DHCP::serialize_list(const list<ipaddress_type> &ip_list) {
    serialization_type buffer(ip_list.size() * sizeof(uint32_t));
    uint32_t *ptr = (uint32_t*)&buffer[0];
    for(list<ipaddress_type>::const_iterator it = ip_list.begin(); it != ip_list.end(); ++it)
        *(ptr++) = *it;
    return buffer;
}

uint32_t DHCP::header_size() const {
    return BootP::header_size() - vend().size() + _size;
}

void DHCP::write_serialization(uint8_t *buffer, uint32_t total_sz, const PDU *parent) {
    assert(total_sz >= header_size());
    if(_size) {
        vend_type &result(BootP::vend());
        result.resize(_size);
        uint8_t *ptr = &result[0] + sizeof(uint32_t);
        // Magic cookie
        *((uint32_t*)&result[0]) = Endian::host_to_be<uint32_t>(0x63825363);
        for(options_type::const_iterator it = _options.begin(); it != _options.end(); ++it) {
            *(ptr++) = it->option();
            *(ptr++) = it->data_size();
            std::copy(it->data_ptr(), it->data_ptr() + it->data_size(), ptr);
            ptr += it->data_size();
        }
    }
    BootP::write_serialization(buffer, total_sz, parent);
}

std::list<DHCP::ipaddress_type> DHCP::generic_search(Options opt, type2type<std::list<ipaddress_type> >) const {
    const dhcp_option *option = search_option(opt);
    if(!option)
        throw option_not_found();
    const uint32_t *ptr = (const uint32_t*)option->data_ptr();
    uint32_t len = option->data_size();
    if((len % sizeof(uint32_t)) != 0)
        throw option_not_found();
    std::list<ipaddress_type> container;
    while(len) {
        container.push_back(ipaddress_type(*(ptr++)));
        len -= sizeof(uint32_t);
    }
    return container;
}

std::string DHCP::generic_search(Options opt, type2type<std::string>) const {
    const dhcp_option *option = search_option(opt);
    if(!option)
        throw option_not_found();
    return string(option->data_ptr(), option->data_ptr() + option->data_size());
}

DHCP::ipaddress_type DHCP::generic_search(Options opt, type2type<ipaddress_type>) const {
    return ipaddress_type(generic_search(opt, type2type<uint32_t>()));
}
}
