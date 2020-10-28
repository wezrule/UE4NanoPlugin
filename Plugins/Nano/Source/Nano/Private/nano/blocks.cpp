// Copyright 2020 Wesley Shillingford. All rights reserved.
#include <nano/blocks.h>

nano::state_hashables::state_hashables (nano::account const & account_a, nano::block_hash const & previous_a, nano::account const & representative_a, nano::amount const & balance_a, nano::uint256_union const & link_a) :
account (account_a),
previous (previous_a),
representative (representative_a),
balance (balance_a),
link (link_a)
{
}

void nano::state_hashables::hash (blake2b_state & hash_a) const
{
	blake2b_update (&hash_a, account.bytes.data (), sizeof (account.bytes));
	blake2b_update (&hash_a, previous.bytes.data (), sizeof (previous.bytes));
	blake2b_update (&hash_a, representative.bytes.data (), sizeof (representative.bytes));
	blake2b_update (&hash_a, balance.bytes.data (), sizeof (balance.bytes));
	blake2b_update (&hash_a, link.bytes.data (), sizeof (link.bytes));
}

nano::state_block::state_block (nano::account const & account_a, nano::block_hash const & previous_a, nano::account const & representative_a, nano::amount const & balance_a, nano::uint256_union const & link_a, nano::raw_key const & prv_a, nano::public_key const & pub_a) :
hashables (account_a, previous_a, representative_a, balance_a, link_a),
signature (nano::sign_message (prv_a, pub_a, hash ()))
{
}

void nano::state_block::hash (blake2b_state & hash_a) const
{
	nano::uint256_union preamble (6);
	blake2b_update (&hash_a, preamble.bytes.data (), preamble.bytes.size ());
	hashables.hash (hash_a);
}

nano::block_hash nano::state_block::hash () const
{
	nano::uint256_union result;
	blake2b_state hash_l;
	auto status (blake2b_init (&hash_l, sizeof (result.bytes)));
	check (status == 0);
	hash (hash_l);
	status = blake2b_final (&hash_l, result.bytes.data (), sizeof (result.bytes));
	check (status == 0);
	return result;
}

nano::signature nano::state_block::block_signature () const
{
	return signature;
}
