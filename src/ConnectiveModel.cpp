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
#include <map>

using namespace std;

struct CCount {

	CCount() : freq(0), sSpread(0), tSpread(0) {}

	CCount& operator++ () {
		freq++;
		return *this;
	}

	CCount operator++ (int) {
		CCount res (*this);
		++(*this);
		return res;
	}

	CCount& operator-- () {
		freq--;
		return *this;
	}

	CCount operator-- (int) {
		CCount res (*this);
		--(*this);
		return res;
	}

	bool empty() {
		return freq <= 0;
	}

	uint freq;
	uint sSpread;
	uint tSpread;

};

struct PhraseMap {

	typedef std::map<Phrase,CCount> PhraseScore_;
	typedef std::map<Phrase,PhraseScore_> PhrasePairMap_;

	PhrasePairMap_ pMap;

	void swap(PhraseMap& other) {
		this->pMap.swap(other.pMap);
	}


	void addPhrasePair(const Phrase& p1, const Phrase& p2) {
		pMap[p1][p2]++;
	}

	void removePhrasePair(const Phrase& p1, const Phrase& p2) {
		pMap[p1][p2]--;
		if (pMap[p1][p2].empty()) {
			pMap[p1].erase(p2);
			if (pMap[p1].size() == 0) {
				pMap.erase(p1);
			}
		}
	}
};

struct ConnectiveModelState : public FeatureFunction::State, public FeatureFunction::StateModifications {
	ConnectiveModelState(uint nsents) : fin(0) {} //

	Float fin;
	PhraseMap s2t;
	PhraseMap t2s;

	Float matchConnectiveDictionary(const AnchoredPhrasePair& pair) {

		 static const boost::regex regSource(".*?\\s?((A|a)fter all|(A|a)fter|(A|a)lso|(A|a)lthough|(B|b)ut|(B|b)ecause|(E|e)ven though|(I|i)nstead|(S|s)ince|(T|t)hough|(M|m)eanwhile|(W|w)hile|(Y|y)et|(H|h)owever|(W|w)hen|(E|e)ven if|(A|a)s if|(I|i)f|(A|a)s soon as|(A|a)s much as|(A|a)s far as|(A|a)s well as|(A|a)s fast as|(J|j)ust as|(A|a)s regards|(A|a)s long as|(A|a)s a result|(A|a)s|(B|b)efore|(T|t)hen|(S|s)till|(U|u)ntil|(T|t)hus|(I|i)n addition|(U|u)nless|(I|i)ndeed|(M|m)oreover|(I|i)n fact|(L|l)ater|(F|f)or example|(O|o)nce|(S|s)eparately|(P|p)reviously|(F|f)inally|(N|n)evertheless|(N|n)onetheless|(B|b)y contrast|(O|o)n the other hand|(S|s)o that|(G|g)iven that|(N|n)ow that|(T|t)herefore|(O|o)therwise|(F|f)or instance|(I|i)n turn)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*");
		 boost::match_results<std::string::const_iterator> results;

		 for (uint j = 0; j < pair.second.get().getSourcePhrase().get().size(); j++) {
			 if (boost::regex_match(pair.second.get().getSourcePhrase().get()[j], results, regSource)) {
				 	 std::string myMatch = results[1];
				 	 // cout << myMatch << endl;
				 	static const boost::regex regTarget(findTargetRegex(myMatch));
				 	if (!boost::regex_match(pair.second.get().getTargetPhrase().get()[j], regTarget)) {
				 		 fin++;
				 		 // cout << "increase " << pair.second.get().getSourcePhrase().get()[j] << " -- " << pair.second.get().getTargetPhrase().get()[j] << endl;
				 	} else {
				 	     fin--;
				 	     // cout << "decrease " << pair.second.get().getSourcePhrase().get()[j] << " deleting " << pair.second.get().getTargetPhrase().get()[j] << endl;
				 	      removePhrasePair(pair); // want to delete (or downscore) the target phrase that actually contains a valid connective translation
				 	}
			 }
		 }

		 // cout << fin << endl;
		return fin;
	}

	void removePhrasePair(const AnchoredPhrasePair& app) {
		Phrase s = app.second.get().getSourcePhrase();
		Phrase t = app.second.get().getTargetPhrase();
		s2t.removePhrasePair(s,t);
		t2s.removePhrasePair(t,s);
	}

	static std::string findTargetRegex(std::string& s) {

		std::string targetRegex;

		if (s == "After all" || s == "after all")
			targetRegex = ".*?\\s?(schliesslich|trotz allem)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "After" || s == "after")
			targetRegex = ".*?\\s?(nachdem|danach|nachher|gemäss|hinterher|später|ab|bis|wenn|vor|hinter|nach)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Also" || s == "also")
			targetRegex = ".*?\\s?(ebenfalls|ausserdem|ebenso|ferner|gleichfalls|ausserdem|gleichzeitig|andererseits|zudem|zugleich|unter anderem|weshalb|aber auch|gleichermassen|darüberhinaus|und auch|auch|und)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Although" || s == "although")
			targetRegex = ".*?\\s?(obwohl|auch wenn|zwar|wenngleich|obgleich|aber|wenn auch|allerdings|jedoch|doch|wobei|während|dennoch|indes|trotzdem|dabei|soweit|trotz)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "But" || s == "but")
			targetRegex = ".*?\\s?(sondern|jedoch|doch|dennoch|aber auch|zwar|auch wenn|dann|nur|dafür|allerdings|obwohl|vielmehr|trotzdem|hingegen|aber)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
	 	else if (s == "Because" || s == "because")
			targetRegex = ".*?\\s?(denn|darum|alldieweil|dieweil|aufgrund|durch||wegen|da|weil)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
	 	else if (s == "Even though" || s == "even though")
	 		targetRegex = ".*?\\s?(obwohl|auch wenn|obgleich|selbst wenn|wenngleich|zwar|trotz|wenn auch|jedoch|während)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
	 	else if (s == "Instead" || s == "instead")
			targetRegex =  ".*?\\s?(anstelle|anstatt dessen|stattdessen|anstatt|statt)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
	 	else if (s == "Since" || s == "since")
			targetRegex = ".*?\\s?(da|seitdem|denn|seither|inzwischen|zumal|angesichts|bisher|wenn|von|seither|ab|mittlerweile|aufgrund|nachdem|anlässlich|nunmehr|nach|seit|weil)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Meanwhile" || s == "meanwhile")
			targetRegex = ".*?\\s?(in der Zwischenzeit|inzwischen|gleichzeitig|unterdessen|währenddessen|während dessen|zwischenzeitlich|mittlerweile|indessen|während|andererseits|dagegen|derweil|derzeit|indes|jedoch)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "While" || s == "while")
			targetRegex = ".*?\\s?(wenngleich|trotzdem|wenn auch|wohingegen|dabei|aber auch|allerdings|ansonsten|hingegen|stattdessen|während gleichzeitig|weiterhin|zur gleichen Zeit|währenddessen|während|zwar|gleichzeitig|obwohl|wobei|aber|jedoch|zugleich|solange|andererseits|auch wenn|bei gleichzeitig|da|wenn|doch|trotz)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Though" || s == "though")
			targetRegex = ".*?\\s?(obwohl|jedoch|auch wenn|allerdings|wenngleich|wenn auch|wobei|dennoch|obgleich|selbst wenn|trotzdem|trotz|aber dennoch|aber doch|gleichwohl|indes|nichtsdestotrotz|sondern|während|zwar|aber|doch)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Yet" || s == "yet")
			targetRegex = ".*?\\s?(dennoch|aber|bisher|trotzdem|jedoch|bislang|erneut|dabei|gleichzeitig|obwohl|andererseits|bereits|bisher noch|derzeit|zunächst einmal|zunächsteinmal|einmal|nach wie vor|nochmals|wenngleich|abermals|bis jetzt|bisjetzt|bisweilen|dagegen|indes|selbst wenn|während|noch immer|noch|doch)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "However" || s == "however")
			targetRegex = ".*?\\s?(jedoch|aber|dagegen|durchaus|indes|hoffentlich|ausserdem|natürlich|allenfalls|andererseits|allerdings|dennoch|trotzdem|hingegen|obwohl|gleichwohl|dabei|jedenfalls|nichtsdestotrotz|nichtsdestoweniger|dessenungeachtet|unterdessen|indessen|nun|zwar|doch|trotz)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "When" || s == "when")
			targetRegex = ".*?\\s?(als|wo|da|wobei|wenn|falls|wann|sobald|während|ob|bis|dann|ehe|neben|nachdem|beim|nach|bei)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "As if" || s == "as if")
			targetRegex = ".*?\\s?(wie wenn|also ob|als wenn|ganz so|so|als)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Even if" || s == "even if")
			targetRegex = ".*?\\s?(auch wenn|selbst wenn|wenn auch)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "If" || s == "if")
			targetRegex = ".*?\\s?(wenn|falls|ob|sofern|im Falle|für den Fall)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "As soon as" || s == "as soon as")
			targetRegex = ".*?\\s?(wenn|sobald wie|so bald wie|sobald|sowie)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "As regards" || s == "as regards")
			targetRegex = ".*?\\s?(was|bezüglich)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Such as" || s == "such as")
			targetRegex = ".*?\\s?(zum Beispiel|beispielsweise|etwa|wie)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Just as" || s == "just as")
			targetRegex = ".*?\\s?(genauso|ebenso wie|ebensowie|gleichwie|ebenso|gerade|ebenfalls)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "As fast as" || s == "as fast as")
			targetRegex = ".*?\\s?(ganz schnell|so schnell|möglichst schnell|so rasch|so bald|sobald)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "As well as" || s == "as well as")
			targetRegex = ".*?\\s?(sowie|und auch|ebenso wie|ebensowie|wie auch|so gut wie|sowohl)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "As far as" || s == "as far as")
			targetRegex = ".*?\\s?(soweit|bis zu|so weit wie|soviel|sofern|bis)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "As much as" || s == "as much as")
			targetRegex = ".*?\\s?(soviel wie|ebenso sehr|ebensosehr|wie auch|so viel|soviel|ebenso viel|ebensoviel|ebenso)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "As a result" || s == "as a result")
			targetRegex = ".*?\\s?(als Ergebnis|dadurch|daher|folglich|daraufhin|demzufolge|dabei|infolgedessen|so|als Folge)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "As long as" || s == "as long as")
			targetRegex = ".*?\\s?(solange wie|solange|solang|sofern|vorausgesetzt, dass|vorausgesetzt)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "As soon as" || s == "as soon as")
			targetRegex = ".*?\\s?(wenn|sobald wie|so bald wie|sobald|sowie)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "As" || s == "as")
			targetRegex = ".*?\\s?(ebenso|gleichwie|während|weil|denn|indem|obgleich|sondern|gleichzeitig|nämlich|dabei|wohingegen|in Form von|wie auch|auch|ob|wenn|um|da|wie|als)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Before" || s == "before")
			targetRegex = ".*?\\s?(bis|bevor|vorher|zuvor|voran|vorn|früher|ehe|nach|vor)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Then" || s == "then")
			targetRegex = ".*?\\s?(danach|anschliessend|damals|darauf|sodann|folglich|alsdann|damalig|denn|derzeitig|anschliessend|nach|dabei|dann|da)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Still" || s == "still")
			targetRegex = ".*?\\s?(immer noch nicht|immer noch|noch immer|dennoch|doch|trotzdem|immerhin|nach wie vor|nachwievor|also|aber|immer noch|weiter|weiterhin|noch)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Until" || s == "until")
			targetRegex = ".*?\\s?(in|bis dass|erst wenn|erst|bislang|bis zu|bis)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Thus" || s == "thus")
			targetRegex = ".*?\\s?(somit|dadurch|daher|deshalb|folglich|demnach|mithin|also|wodurch|ebenfalls|hiermit|so)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "In addition" || s == "in addition")
			targetRegex = ".*?\\s?(zusätzlich|ausserdem|dazu|daneben|ferner|hinzu|des Weiteren|zuzüglich|ausserdem|nicht nur|darüber hinaus|darüberhinaus|ausser|auch|und|noch)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Unless" || s == "unless")
			targetRegex = ".*?\\s?(ausser wenn|es sei denn|sofern nicht|sofern|wenn nicht|falls nicht|solange wie|vorausgesetzt, dass|ausgenommen dass|wenn)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Indeed" || s == "indeed")
			targetRegex = ".*?\\s?(in der Tat|zwar|ja|allerdings|gewiss|tatsächlich|wirklich|wohl|wahrlich|freilich)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Moreover" || s == "moreover")
			targetRegex = ".*?\\s?(ausserdem|zudem|ferner|zusätzlich|überdies|darüber hinaus|sonst|dabei|auch|ja|dagegen|sogar|übrigens)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "In fact" || s == "in fact")
			targetRegex = ".*?\\s?(tatsächlich|in der Tat|eigentlich|und zwar|vielmehr|in Wirklichkeit|wahrhaftig|so gesehen|sogar|faktisch|übrigens|in Wahrheit)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Later" || s == "later")
			targetRegex = ".*?\\s?(später|nachträglich|nachher|nachmals|darauf|nach)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "For example" || s == "for example")
			targetRegex = ".*?\\s?(zum Beispiel|beispielsweise|etwa|unter anderem|so)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "For instance" || s == "for instance")
			targetRegex = ".*?\\s?(zum Beispiel|beispielsweise|etwa)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Once" || s == "once")
			targetRegex = ".*?\\s?(sobald|einst|ehemals|wenn erst einmal|einmal|weiland|danach|seit|damals|als|wenn)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Separately" || s == "separately")
			targetRegex = ".*?\\s?(getrennt|gesondert|separat|einzeln|extra|besonders|für sich|hiervon unabhängig)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Previously" || s == "previously")
			targetRegex = ".*?\\s?(zuvor|ehemals|vorher|früher)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Finally" || s == "finally")
			targetRegex = ".*?\\s?(schliesslich|endgültig|am Ende|zum Schluss|zum Abschluss|letztendlich|zuletzt|schlussendlich|zu guter Letzt|schon|endlich)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Nevertheless" || s == "nevertheless")
			targetRegex = ".*?\\s?(dennoch|trotzdem|gleichwohl|nichtsdestotrotz|des ungeachtet|desungeachtet|nichtsdestoweniger|dessen ungeachtet|dessenungeachtet|immerhin|nur|dann|allerdings|doch)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Nonetheless" || s == "nonetheless")
			targetRegex = ".*?\\s?(dennoch|trotzdem|gleichwohldoch|nichtsdestotrotz|nichtsdestoweniger|gleichviel|nichtsdestominder|so|aber)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "In turn" || s == "in turn")
			targetRegex = ".*?\\s?(wiederum|im Gegenzug|der Reihe nach|reihum)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "By contrast" || s == "by contrast")
			targetRegex = ".*?\\s?(hingegen|dagegen|im Gegensatz zu)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "On the other hand" || s == "on the other hand")
			targetRegex = ".*?\\s?(andererseits|wiederum|demgegenüber|andrerseits|dahingegen|hingegen|dagegen|hinwieder)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "So that" || s == "so that")
			targetRegex = ".*?\\s?(so dass|damit|sodass|so|weshalb)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Given that" || s == "given that")
			targetRegex = ".*?\\s?(zumal|aufgrund|weil|angesichts|wenn man bedenkt, dass|da)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Now that" || s == "now that")
			targetRegex = ".*?\\s?(aufgrund|nachdem|jetzt|nunmehr|nun|nachdem|da|währenddessen|obwohl)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Therefore" || s == "therefore")
			targetRegex = ".*?\\s?(daher|deshalb|somit|also|deswegen|darum|folglich|dafür|hierfür|demzufolge|mithin|ergo|aus diesem Grund|damit|demnach|infolgedessen)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		else if (s == "Otherwise" || s == "otherwise")
			targetRegex = ".*?\\s?(ansonsten|andernfalls|anderweitig|anderenfalls|anderweit|andererseits|widrigenfalls|andrerseits|anders|sonst)(\\s|\\,|\\.|\\?|\\!|\\:|\\;|\\'|\\’|$).*";
		return targetRegex;
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
	*sbegin = Float(1);
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
	os->s2t.swap(ms->s2t);
	return oldState;
}
