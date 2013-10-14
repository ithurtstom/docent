/*
 *  ConnectiveModel.cpp
 *
 *  Copyright 2013 by Thomas Meyer. All rights reserved.
 *
 *  This file is part of Docent, a document-level decoder for phrase-based
 *  statistical machine translation.
 *
 *  Docent is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  Docent is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  Docent. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Docent.h"
#include "DocumentState.h"
#include "FeatureFunction.h"
#include "SearchStep.h"
#include "ConnectiveModel.h"

#include <boost/regex.hpp>

#include <iostream>
#include <string>

using namespace std;

struct ConnectiveModelState : public FeatureFunction::State, public FeatureFunction::StateModifications {
	ConnectiveModelState(uint nsents) : outputLength(nsents) {}

	std::vector<uint> outputLength;

	Float matchConnectiveDictionary(const AnchoredPhrasePair& pair) {

		char* const reg_source = ".*?(W|w)hile.*";
		char* const reg_target = ".*?(wenngleich|trotzdem|wenn auch|wohingegen|dabei|aber auch|allerdings|ansonsten| hingegen|stattdessen|während gleichzeitig|weiterhin|zur gleichen zeit|währenddessen|während|zwar|gleichzeitig|obwohl|wobei|aber|jedoch|zugleich|solange|andererseits|auch wenn|bei gleichzeitig|da|wenn|doch|trotz).*";

		 boost::regex regSrc (reg_source);
		 boost::regex regTrg (reg_target);

		Float fin = 0.00;

		std::vector<Word> src = pair.second.get().getSourcePhrase().get();
		std::vector<Word> trg = pair.second.get().getTargetPhrase().get();


	for (uint j = 0; j < src.size(); j++) {
		// BOOST_FOREACH(const Word &w, src) {
		// BOOST_FOREACH(const Word &u, trg) {
			if (boost::regex_match(src[j], regSrc)) {
				if (!boost::regex_match(trg[j], regTrg)) {
					fin++;
				} else {
					fin--;
				}
			}
		// }
		// }
	}
		// cout << fin << endl;
		return fin;
	}

	virtual ConnectiveModelState *clone() const {
		return new ConnectiveModelState(*this);
	}
};

FeatureFunction::State *ConnectiveModel::initDocument(const DocumentState &doc, Scores::iterator sbegin) const {
	const std::vector<PhraseSegmentation> &segs = doc.getPhraseSegmentations();

	ConnectiveModelState *s = new ConnectiveModelState(segs.size());
	for(uint i = 0; i < segs.size(); i++) {
		BOOST_FOREACH(const AnchoredPhrasePair &pair, segs[i]) {
			*sbegin = s->matchConnectiveDictionary(pair);
		}
	}
	return s;
}

void ConnectiveModel::computeSentenceScores(const DocumentState &doc,uint sentno,Scores::iterator sbegin) const {
	*sbegin = Float(0);
}

FeatureFunction::StateModifications *ConnectiveModel::estimateScoreUpdate(const DocumentState &doc, const SearchStep &step, const State *state,
																			 Scores::const_iterator psbegin, Scores::iterator sbegin) const {
	const ConnectiveModelState *prevstate = dynamic_cast<const ConnectiveModelState *>(state);
	ConnectiveModelState *s = prevstate->clone();

	const std::vector<SearchStep::Modification> &mods = step.getModifications();
	for(std::vector<SearchStep::Modification>::const_iterator it = mods.begin(); it != mods.end(); ++it) {
			BOOST_FOREACH(const AnchoredPhrasePair &pair, it->proposal) {
				*sbegin = s->matchConnectiveDictionary(pair);
			}

	}
	return s;
}

FeatureFunction::StateModifications *ConnectiveModel::updateScore(const DocumentState &doc, const SearchStep &step, const State *state,
																	 FeatureFunction::StateModifications *estmods, Scores::const_iterator psbegin, Scores::iterator estbegin) const {
	return estmods;
}

FeatureFunction::State *ConnectiveModel::applyStateModifications(FeatureFunction::State *oldState, FeatureFunction::StateModifications *modif) const {
	ConnectiveModelState *os = dynamic_cast<ConnectiveModelState *>(oldState);
	ConnectiveModelState *ms = dynamic_cast<ConnectiveModelState *>(modif);
	os->outputLength.swap(ms->outputLength);
	return oldState;
}
