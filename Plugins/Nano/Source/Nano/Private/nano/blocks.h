// Copyright 2020 Wesley Shillingford. All rights reserved.
#pragma once

#include <nano/numbers.h>

#ifdef _WIN32
#pragma warning (disable : 4804 ) /* '/': unsafe use of type 'bool' in operation warnings */
#endif
#include <blake2/blake2.h>

namespace nano
{

class state_hashables final
{
public:
	state_hashables () = default;
	state_hashables (nano::account const &, nano::block_hash const &, nano::account const &, nano::amount const &, nano::uint256_union const &);
	void hash (blake2b_state &) const;
	nano::account account;
	nano::block_hash previous;
	nano::account representative;
	nano::amount balance;
	nano::uint256_union link;
};

class state_block final
{
public:
	state_block () = default;
	state_block (nano::account const &, nano::block_hash const &, nano::account const &, nano::amount const &, nano::uint256_union const &, nano::raw_key const &, nano::public_key const &);
	void hash (blake2b_state &) const;
	nano::signature block_signature () const;
	nano::state_hashables hashables;
	nano::block_hash hash () const;

private:
	nano::signature signature;
};
}